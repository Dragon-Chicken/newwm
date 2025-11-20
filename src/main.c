#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "defs.h"

void keypress(XEvent *ev) {
  XKeyEvent *xkey = &ev->xkey;
  if (KeysymToString(xkey) == 'q' && xkey->state == Mod1Mask) {
    exit(0);
  }
}

void maprequest(XEvent *ev) {
  XMapRequestEvent *mapreq = &ev->xmaprequest;
  
  Tile *newtile = (Tile *)malloc(sizeof(Tile));
  newtile->win = mapreq->window;

  /* structure:
   * each index is a tile
   * each tile has both a parent and the next tile
   *
   * head/1 (parent NULL, next 2)
   * 2 (parent 1, next 3)
   * 3 (parent 2, next 4)
   * 4 (parent 1, next NULL) << IMPORTANT!
   *
   * loop through them like:
   * for (thistile = head; thistile = thistile->next; thistile->next != NULL)
   */

  if (headtile == NULL) {
    // set this tile to the head
    headtile = newtile;
    newtile->parent = NULL;
    newtile->next = NULL;

  } else {
    // add this tile to the end of the linked list
    // so, go to latest tile

    Tile *tile = headtile;
    while (tile->next != NULL) {
      tile = tile->next;
    }
    tile->next = newtile;
    newtile->parent = tile; // this code WILL NEED TO change depending on the layout
                            // better idea is to put the parent to the currently selected tile
    newtile->next = NULL;
  }

  printf("linked list:\n");
  for (Tile *tile = headtile; tile != NULL; tile = tile->next) {
    printf("%lx\n", tile->win);
  }

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

void destroynotify(XEvent *ev) {
  /* the wm doesn't have to do anything other than retile the remaining windows
   * becuase this is a notify event not a request */
  Window destroywin = ev->xdestroywindow.window;

  // find this win in the linked list of tiles
  Tile *prev = NULL;
  Tile *tile = headtile;
  while (tile != NULL && tile->win != destroywin) {
    prev = tile;
    tile = tile->next;
  }

  if (tile) {
    printf("found win\n");
    printf("tile: %x, win: %lx\n", tile, tile->win);
    if (headtile == tile) {
      headtile = tile->next;
    }
    if (prev) {
      prev->next = tile->next;
    }
    free(tile);
  }

  printf("linked list:\n");
  for (Tile *tile = headtile; tile != NULL; tile = tile->next) {
    printf("%lx\n", tile->win);
  }
}

void void_event(XEvent *ev) {
  // do nothing
}


char KeysymToString(XKeyEvent *xkey) {
  return *XKeysymToString(XLookupKeysym(xkey, 0));
}

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

void setup(void) {
  headtile = NULL;

  for (int i = 0; i < LASTEvent; i++) {
    handler[i] = void_event;
  }
  handler[KeyPress] = keypress;
  handler[MapRequest] = maprequest;
  handler[DestroyNotify] = destroynotify;
}

void master_stack_tile(void) {
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
  XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);

  // asking ONLY for alt + a input events
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("q")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);

  printf("Default screen: %d\nScreen width: %d\nScreen height: %d\n", screen, screenw, screenh);

  setup_atoms();
  setup();

  for (;;) {
    XNextEvent(dpy, &ev);
    printf("event rec of type %d\n", ev.type);

    handler[ev.type](&ev);
  }

  XCloseDisplay(dpy);
  printf("exiting with no errors\n");

  return 0;
}
