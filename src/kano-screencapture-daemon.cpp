// kano-screencapture-daemon
//
// Copyright (C) 2017 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// Daemon which waits for share key presses and launches pop-ups and the capture command
//
//   TODO:
//     - complete error checking
//     - fix logging
//     - get correct images
//     - fix screencapture command to deal with resolutions properly
//     - add call to avconv and put file in users folder


#include <sys/signalfd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include "libinput_wrap.h"
#include "process.h"


#define IMAGE_PATH_BASE "/usr/share/icons/Kano/88x88/apps/"
#define IMAGE_TIMEOUT " 3 "
#define IMAGE_CMD_BASE "kano-start-splash -b 0x0000 -t " IMAGE_TIMEOUT IMAGE_PATH_BASE


// TBD: fix placeholder images
#define IMAGE_CMD_PROMPT IMAGE_CMD_BASE "snake.png"
#define IMAGE_CMD_TIMEOUT IMAGE_CMD_BASE "snake.png"
#define IMAGE_CMD_EXIT IMAGE_CMD_BASE "snake.png"
#define IMAGE_STOP_CMD "kano-stop-splash"

// TBD: don't fix resolution
#define RECORD_COMMAND "kano-screencapture -h 1024 -w 768 -r 25 -f 7500 -o /tmp/cap.h264"

#define KEY_FN_SHARE 185

void main_loop(void)
{
    // Loop:
    //   Wait for share key
    //   When pressed, pop up prompt for 3 seconds
    //   If share key is pressed before timeout:
    //      start capture
    //      if share key is again pressed before timeout
    //         stop capture
    //         pop up exit prompt for 3 seconds
    //      else:
    //         pop up timeout prompt for 3 seconds
    //      (TBD) postprocess and copy output
    
    int child_pid = -1;
    int status;
    while(true)
    {
        relevant_event event = EVENT_NONE;
        while(event != EVENT_SHARE_KEY_RELEASE)        
            event = yield();

        child_pid = process_start(IMAGE_CMD_PROMPT);
        if(child_pid == -1){
            printf("error starting child");
            continue;
        }

        event = yield();

        if(event == EVENT_CHILD_DIED || event == EVENT_NONE)
        {
            if(event  == EVENT_CHILD_DIED)
            {
                
                waitpid(child_pid, &status, WNOHANG); // release popup process
                // todo - check status
            }
            
            // share key not pressed again; therefore go back to waiting
            continue;
        }
        
        // kill the popup and start recording
        system(IMAGE_STOP_CMD);

        child_pid = process_start(RECORD_COMMAND);
        if(child_pid == -1){
            printf("error starting child");
            continue;
        }
        const char *final_popup_cmd = IMAGE_CMD_TIMEOUT;
        event = yield();
        if(event == EVENT_SHARE_KEY_RELEASE)
        {
            // key pressed again, stop the recording
            final_popup_cmd = IMAGE_CMD_EXIT;
            kill(child_pid, SIGINT);
            event = yield();
        }
        while(event != EVENT_CHILD_DIED)
        {
            event = yield();
            // TBD if we get stuck here, try kill -9?
        }
        waitpid(child_pid, &status, WNOHANG); // release capture process
        // todo - check status


        child_pid = process_start(final_popup_cmd);
        if(child_pid == -1){
            printf("error starting child");
            continue;
        }

        event = yield();
        while(event != EVENT_CHILD_DIED)
        {
            event = yield();
        }

        waitpid(child_pid, &status, WNOHANG); // release popup process
        // todo - check status

        
    }
}


void processKeyEvent(struct libinput_event *ev)
{
    
    enum libinput_event_type type = libinput_event_get_type(ev);
    struct libinput_device *dev = libinput_event_get_device(ev);
    switch (type) {
    case LIBINPUT_EVENT_DEVICE_ADDED:
        if(libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_KEYBOARD)){
            printf("new keyboard\n");
        }
        break;
    case LIBINPUT_EVENT_DEVICE_REMOVED:
        if(libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_KEYBOARD)){
            printf("dead keyboard\n");
        }
        break;
        
    case LIBINPUT_EVENT_KEYBOARD_KEY:
        {
            struct libinput_event_keyboard* kev = libinput_event_get_keyboard_event(ev);
            uint32_t keycode = libinput_event_keyboard_get_key(kev);
            enum libinput_key_state state = libinput_event_keyboard_get_key_state(kev);        
            printf("key %d %d\n", keycode, state);
            if(keycode == KEY_FN_SHARE && state == 0)
            {
                send_event(EVENT_SHARE_KEY_RELEASE);
            }
            break;
        }
    default:
        break;
    }
}


void processSignalEvent(struct signalfd_siginfo *si)
{
    printf("got signal %d, waiting for %d\n", si->ssi_signo, SIGCHLD);
    if(si->ssi_signo == SIGCHLD)
    {
        send_event(EVENT_CHILD_DIED);
    }
}


int main(int argc, char * argv[])
{
    init_coroutines(main_loop);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        printf("error in sigprocmask\n");
        exit(1);
    }
    int sigFd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    libinput_wrap_state *st = libinput_wrap_init();

    int liFd = libinput_wrap_get_fd(st);
    fd_set readFds;

    if(!st){
        printf("libinput_wrap failed");
        exit(1);
    }
                                     
    
    
    if(!libinput_wrap_process_events(st, processKeyEvent))
    {
        while(true){
            FD_ZERO(&readFds);
            FD_SET(liFd, &readFds);
            FD_SET(sigFd, &readFds);
            select(MAX(liFd,sigFd)+1, &readFds, NULL, NULL, NULL);
            if(FD_ISSET(liFd, &readFds)){
                if(libinput_wrap_process_events(st, processKeyEvent))
                    break;
                
            }
            if(FD_ISSET(sigFd, &readFds)){
                struct signalfd_siginfo si;
                int count = read(sigFd, &si, sizeof(si));
                if(count == sizeof(si))
                {
                    processSignalEvent(&si);
                }
            }
        }
    }
    else
    {
        printf("libinput_wrap_error");        
    }
        
    libinput_wrap_close(st);    
    
}
