#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>

Display *dpy;
Window root;

int screenw, screenh;

int main() {
  XEvent ev;

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
  //XSelectInput(dpy, root, SubstructureRedirectMask | KeyPressMask);

  // asking ONLY for alt + a input events
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("a")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);

  printf("Default screen: %d\nScreen width: %d\nScreen height: %d\n", screen, screenw, screenh);

  while (true) {
    XNextEvent(dpy, &ev);
    switch (ev.type) {
      case KeyPress:
        printf("alt + a was pressed\n");
        break;
    }
  }

  XCloseDisplay(dpy);
  printf("exiting with no errors\n");

  return 0;
}
