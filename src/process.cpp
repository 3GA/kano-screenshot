// process.cpp 
//
// Copyright (C) 2017 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
//  Routines to:
//    - launch a command asyncronously
//    - run an event-triggered coroutine (only one allowed)

// TBD: move cleanup of asynchronous command here too.

#include <unistd.h>
#include <sys/types.h>
#include <sys/signalfd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
extern "C" {
#include <ucontext.h>
}

#include "process.h"

#define PAGE_SIZE  4096
#define STACK_SIZE PAGE_SIZE*4


ucontext_t main_coroutine;
ucontext_t event_coroutine;
relevant_event next_event;


void fatal(const char *err)
{
    puts(err);
    exit(1);
}

int run(void *cmd) {
    printf("executing cmd %s\n", cmd);
    return system((char *)cmd);
}


int process_start(const char *cmd) {
  int flags = CLONE_FS | CLONE_IO | SIGCHLD;
  char * stack = (char *)mmap(NULL, STACK_SIZE , \
		      PROT_READ | PROT_WRITE, MAP_PRIVATE | \
		      MAP_ANON | MAP_GROWSDOWN, -1, 0);

  if(stack==MAP_FAILED) fatal("unable to obtain stack\n");
  long pid = clone(run,(int *)(stack+STACK_SIZE),flags,cmd,NULL,NULL,NULL);
  return pid;
}


void init_coroutines(void (*fn)())
{
    getcontext(&main_coroutine);
    char * stack = (char *)mmap(NULL, STACK_SIZE ,                       \
                                PROT_READ | PROT_WRITE, MAP_PRIVATE |   \
                                MAP_ANON | MAP_GROWSDOWN, -1, 0);
    event_coroutine.uc_stack.ss_sp = stack;
    event_coroutine.uc_stack.ss_size = STACK_SIZE;
    event_coroutine.uc_link = &main_coroutine;
    makecontext(&event_coroutine, fn, 0);
    send_event( EVENT_NONE);
}

                     
void send_event(relevant_event event)
{
    next_event = event;
    swapcontext(&main_coroutine, &event_coroutine);
}
void trappoint(void){
    // breakpoint on this function to get the debugger
    // to see when the coroutine is entered
    // (Otherwise gdb just skips over it)
}
    
relevant_event yield(void){
    swapcontext(&event_coroutine, &main_coroutine);
    trappoint();
    return next_event;   
}
