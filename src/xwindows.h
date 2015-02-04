//
// xwindows.h
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//

#include <X11/Xlib.h>

bool compare_window_name (Display *display, Window w, char *window_name, bool verbose);
bool get_window_geometry (Display *display, Window w, int *x, int *y, int *cx, int *cy);
bool findWindowCoordinatesByName (char *window_name, bool verbose, int *x, int *y, int *cx, int *cy);
