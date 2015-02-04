//
// xwindows.c
//
// Copyright (C) 2014, 2015 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// Module to collect X11 window placement coordinates given by name property.
// Assists kano-screenshot in cropping application areas from the desktop
//

#include <X11/Xlib.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Given a window w and a window name, tell us if it's title is the same.
bool compare_window_name (Display *display, Window w, char *window_name, bool verbose)
{
  bool success=false;
  char *enumerated_windowname=NULL;

  if (XFetchName (display, w, &enumerated_windowname)) {
    if (verbose) {
      printf ("Window id 0x%x (%s)\n", (unsigned int) w, enumerated_windowname);
    }

    if (window_name && !strncmp (enumerated_windowname, window_name, strlen (window_name))) {
      XFree (enumerated_windowname);
      success = true;
    }
  }

  return success;
}

// Given a window w, return its placement coordinates and size
bool get_window_geometry (Display *display, Window w, int *x, int *y, int *cx, int *cy)
{
  Window root;
  Status rcsuccess=0;
  int rx=0, ry=0;
  unsigned int rcx=0, rcy=0;
  bool success=false;

  // FIXME: Window decorations might affet below spacing?
  unsigned int border_width=0, depth=0;

  rcsuccess = XGetGeometry (display, w, &root, &rx, &ry, &rcx, &rcy, &border_width, &depth);
  if (rcsuccess) {
    // We've got the window coordinates, send them back and leave.
    if (x)  *x  = rx;
    if (y)  *y  = ry;
    if (cx) *cx = rcx;
    if (cy) *cy = rcy;
    success = true;
  }
  
  return success;
}

bool findWindowCoordinatesByName (char *window_name, bool verbose, int *x, int *y, int *cx, int *cy)
{
  Display *display=NULL;
  Window returnedroot=0L, returnedparent=0L, root=0L;
  Window *children=NULL, *subchildren=NULL;
  unsigned int numchildren=0, numsubchildren=0;
  int i=0, k=0;

  display = XOpenDisplay (NULL);
  if (!display) {
    printf ("Could not connect to the XServer - is your DISPLAY variable set?\n");
    return false;
  }
  
  root = DefaultRootWindow(display);
  XQueryTree (display, root, &returnedroot, &returnedparent, &children, &numchildren);
  for (i=numchildren-1; i >= 0; i--) {
    if (compare_window_name (display, children[i], window_name, verbose) == true) {
      get_window_geometry (display, children[i], x, y, cx, cy);
      XFree (children);
      XCloseDisplay (display);
      return true;
    }

    // Search one level deeper in the hierarchy
    XQueryTree (display, children[i], &returnedroot, &returnedparent, &subchildren, &numsubchildren);
    for (k=numsubchildren-1; k >= 0; k--) {

      if (compare_window_name (display, subchildren[k], window_name, verbose) == true) {
	get_window_geometry (display, children[i], x, y, cx, cy);
	XFree (subchildren);
	XCloseDisplay (display);
	return true;
      }
    }
  }

  XCloseDisplay (display);
  return false;
}
