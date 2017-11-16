/*
 * libinput_wrap.c
 *
 * Copyright (C) 2016 Kano Computing Ltd.
 * Copyright (C) 2016 The Qt Company Ltd.
 * Contact: https://www.qt.io/licensing/
 * License: http://www.gnu.org/licenses/gpl-2.0.txt LGPL v2
 *
 * This module wraps libinput. It is derived from qlibinputhandler.cpp
 * and is therefore subject to the LGPL rather than GPL.
 *
 */



#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "libinput_wrap.h"


static int liOpen(const char *path, int flags, void *user_data)
{
    return open(path, flags);
}

static void liClose(int fd, void *user_data)
{
    close(fd);
}
static void liLogHandler(struct libinput *libinput, enum libinput_log_priority priority, const char *format, va_list args)
{
    printf( format, args);
}


static const struct libinput_interface liInterface = {
    liOpen,
    liClose
};



void processEvent(struct libinput_event *ev)
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
        break;
    }
    default:
      break;
    }
    
}

void libinput_wrap_close(libinput_wrap_state *st)
{
    if(!st)
        return;

    if(st->m_udev)
    {
        udev_unref(st->m_udev);
        st->m_udev = NULL;        
    }
    if(st->m_li)
    {
        libinput_unref(st->m_li);
        st->m_li = NULL;
    }
    free(st);
}


libinput_wrap_state *libinput_wrap_init(void)
{
    libinput_wrap_state *st =  (libinput_wrap_state *)malloc(sizeof(libinput_wrap_state));
    if(!st){
        printf("malloc failed\n");
        return NULL;
    }
    memset((void *)st, 0, sizeof(st));
    st->m_udev = udev_new();
    if(!st->m_udev)
    {
        printf("udev fail\n");
        libinput_wrap_close(st);
        return NULL;
    }
    st->m_li = libinput_udev_create_context(&liInterface, NULL, st->m_udev);
    if(!st->m_li)
    {
        printf("libinput open fail\n");
        libinput_wrap_close(st);
        return NULL;
    }
    
    libinput_log_set_handler(st->m_li, liLogHandler);

    if(libinput_udev_assign_seat(st->m_li, "seat0"))
    {
        printf("assign fail\n");
        libinput_wrap_close(st);
        return NULL;
    }
    st->fd = libinput_get_fd(st->m_li);

    return st;
}

int libinput_wrap_get_fd(libinput_wrap_state *st)
{
    if(st)
    {
        return st->fd;
    }
    else
    {
        return -1;
    }
}

int libinput_wrap_process_events(libinput_wrap_state *st, void (eventCB)(struct libinput_event *ev))
{
    if (libinput_dispatch(st->m_li)){
        printf("dispatch failed\n");
        return -1;
    }
    struct libinput_event *ev;
    while ((ev = libinput_get_event(st->m_li)) != NULL) {
        eventCB(ev);
        libinput_event_destroy(ev);
    }
    return 0;
}

#ifdef TEST
int main(int argc, char * argv[])
{

    libinput_wrap_state *st = libinput_wrap_init();
    if(!st){
        printf("libinput_wrap failed");
        exit(1);
    }
                                     
    
    int m_liFd = libinput_wrap_get_fd(st);
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(m_liFd, &readFds);
    

    while(true){
        if(libinput_wrap_process_events(st, processEvent))
            break;
        select(m_liFd+1, &readFds, NULL, NULL, NULL);
    }

    libinput_wrap_close(st);

}
#endif
