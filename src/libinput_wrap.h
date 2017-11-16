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

#ifndef LIBINPUT_WRAP_H
#define LIBINPUT_WRAP_H

#include <libudev.h>
#include <libinput.h>

typedef struct
{
    struct libinput *m_li;
    struct udev *m_udev;
    int fd;
}libinput_wrap_state;  

libinput_wrap_state *libinput_wrap_init(void);
int libinput_wrap_get_fd(libinput_wrap_state *st);
int libinput_wrap_process_events(libinput_wrap_state *st, void (eventCB)(struct libinput_event *ev));
void libinput_wrap_close(libinput_wrap_state *st);

#endif
