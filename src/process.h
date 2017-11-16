// process.cpp 
//
// Copyright (C) 2017 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
//  Routines to:
//    - launch a command asyncronously
//    - run an event-triggered coroutine (only one allowed)

#ifndef PROCESS_H
#define PROCESS_H


// start command in parallel, returning its PID
int process_start(const char *cmd);
void init_coroutines(void (*fn)());

typedef enum
{
    EVENT_NONE,
    EVENT_SHARE_KEY_RELEASE,
    EVENT_CHILD_DIED
} relevant_event;
    

void send_event(relevant_event event);
relevant_event yield(void);


#endif
