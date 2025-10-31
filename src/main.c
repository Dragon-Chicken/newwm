#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "defs.h"

Display *dpy;
Window root;

int screenw, screenh;

Atom _NET_SUPPORTING_WM_CHECK;
Atom _NET_NUBER_OF_DESKTOPS;
Atom _NET_WM_NAME;

Atom UTF8_STRING;

// TEMP
// each virtual desktop needs it's own mapped count
int totalmapped;

char KeysymToString(XKeyEvent *xkey) {
  return *XKeysymToString(XLookupKeysym(xkey, 0));
}

void key_press(XKeyEvent *xkey) {
  if (KeysymToString(xkey) == 'q' && xkey->state == Mod1Mask) {
    exit(0);
  }
}

// atoms are workspaces or something idk
void setup_atoms(void) {
  _NET_SUPPORTING_WM_CHECK = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  _NET_WM_NAME = XInternAtom(dpy, "_NET_WM_NAME", False);
  _NET_NUBER_OF_DESKTOPS = XInternAtom(dpy, "_NET_NUBER_OF_DESKTOPS", False);

  UTF8_STRING = XInternAtom(dpy, "UTF8_STRING", False);

  // EWMH needs a child window for hints
  // wm name other props are in that child win
  Window WmCheckWin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
  // make new win the child of root
  XChangeProperty(dpy, root, _NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
      PropModeReplace, (unsigned char*) &WmCheckWin, 1);
  // make new win the child of itself
  XChangeProperty(dpy, WmCheckWin, _NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
      PropModeReplace, (unsigned char*) &WmCheckWin, 1);

  char *wmname = WM_NAME;
  XChangeProperty(dpy, WmCheckWin, _NET_WM_NAME, UTF8_STRING, 8,
      PropModeReplace, (unsigned char*) wmname, strlen(wmname));
}

void map_window(XMapRequestEvent *mapreq) {
  totalmapped++;
  if (totalmapped==1) {
    XMoveResizeWindow(dpy, mapreq->window,
        0, 0, screenw/2, screenh);
  } else {
    XMoveResizeWindow(dpy, mapreq->window,
        screenw/2, 0, screenw/2, screenh);
  }

  XMapWindow(dpy, mapreq->window);
}

int main() {
  XEvent ev;
  totalmapped = 0;

  if (!(dpy = XOpenDisplay(NULL))) {
    printf("failed to open display");
    exit(1);
  }

  int screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  screenw = XDisplayWidth(dpy, screen);
  screenh = XDisplayHeight(dpy, screen);

  // type of events we'll be handling
  // use https://tronche.com/gui/x/xlib/events/processing-overview.html
  // if you don't want to killl yourself
  XSelectInput(dpy, root, SubstructureRedirectMask);

  // asking ONLY for alt + a input events
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("q")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);

  printf("Default screen: %d\nScreen width: %d\nScreen height: %d\n", screen, screenw, screenh);

  setup_atoms();

  while (true) {
    XNextEvent(dpy, &ev);

    printf("\n------------------\n");
    printf("event rec of type %d\n", ev.type);

    switch (ev.type) {
      case KeyPress:
        printf("KeyPress:\n");
        printf("key: %c\n", KeysymToString(&ev.xkey));
        printf("mask: %d\n", ev.xkey.state);
        key_press(&ev.xkey);
        break;
      case MapRequest:
        printf("MapRequest\n");
        map_window(&ev.xmaprequest);
        break;
    }
  }

  XCloseDisplay(dpy);
  printf("exiting with no errors\n");

  return 0;
}
