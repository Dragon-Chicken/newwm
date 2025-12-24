#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"
#include "defs.h"

void voidevent(XEvent *ev) {
  printf("(void event)\n");
}

void keypress(XEvent *ev) {
  printf("(keypress)\n");

  XKeyEvent *xkey = &ev->xkey;
  if (KeysymToString(xkey) == 'q' && xkey->state == Mod1Mask) {
    exit(0);
  }
  // spawn st
  if (KeysymToString(xkey) == 'a' && xkey->state == Mod1Mask) {
    spawn();
  }
}

void maprequest(XEvent *ev) {
  printf("(maprequest)\n");

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

  // should prob always init these two to NULL
  newtile->parent = NULL;
  newtile->next = NULL;

  if (headtile == NULL) {
    headtile = newtile;
  } else {
    // add this tile to the end of the linked list
    // so, go to latest tile
    Tile *tile = headtile;
    while (tile->next != NULL)
      tile = tile->next;

    tile->next = newtile;
    newtile->parent = tile; // this code WILL NEED TO change depending on the layout
                            // better idea is to put the parent to the currently selected tile
  }

  /*printf("linked list:\n");
  for (Tile *tile = headtile; tile != NULL; tile = tile->next) {
    printf("%lx\n", tile->win);
  }*/

  master_stack_tile();

  focused = headtile;
}

void destroynotify(XEvent *ev) {
  printf("(destroynotify)\n");
  /* the wm doesn't have to do anything other than retile the remaining windows
   * becuase this is a notify event not a request */
  Window destroywin = ev->xdestroywindow.window;

  /*
   * THIS DOES NOT HANDLE THE CASE WHERE A TILE WHO HAS CHILDREN GETS DELETED,
   * THOSE CHILDREN STILL THINK THIS TILE EXISTS
   */

  // find this win in the linked list of tiles
  Tile *prev = NULL;
  Tile *tile = headtile;
  while (tile != NULL && tile->win != destroywin) {
    prev = tile;
    tile = tile->next;
  }

  Tile *deletetile = tile;

  if (tile) {
    printf("destroy: ");
    printf("tile: %x, win: %lx\n", tile, tile->win);
    if (headtile == tile) {
      headtile = tile->next;
    }
    if (prev) {
      prev->next = tile->next;
    }
  } else {
    printf("ERROR: cannot find tile/window\n");
  }

  while (tile != NULL) {
    if (tile->parent == deletetile) {
      if (deletetile->parent == NULL) {
        deletetile->parent = tile;
      }
      tile->parent = deletetile->parent;
    }
    tile = tile->next;
  }

  if (deletetile == focused) {
    focused = deletetile->parent;
    printf("focusing win: %lx\n", focused->win);
    XSetInputFocus(dpy, focused->win, RevertToPointerRoot, CurrentTime);
  }

  // freeing the tile
  free(deletetile);

  // tile
  master_stack_tile();

  /*printf("linked list:\n");
  for (Tile *tile = headtile; tile != NULL; tile = tile->next) {
    printf("%lx\n", tile->win);
  }*/
}

void focusin(XEvent *ev) {
  printf("(focusin)\n");

  if (!focused)
    return;

  if (focused && ev->xfocus.window != focused->win) {
    printf("focusing win: %lx\n", focused->win);
    XSetInputFocus(dpy, focused->win, RevertToPointerRoot, CurrentTime);
  }
}


void master_stack_tile(void) {
  /* makes one master window
   * to the right is a stack of newer windows*/
  /* needs to check each tiles parent
   * and each time half the parents width and put this tile to the right of the parent (for now)*/

  for (Tile *tile = headtile; tile != NULL; tile = tile->next) {
    // head case
    if (tile == headtile) {
      tile->x = 0; tile->y = 0;
      tile->w = screenw; tile->h = screenh;
      continue;
    }

    // don't tile parentless tiles
    // or maybe make them float?
    if (tile->parent == NULL) {
      continue;
    }

    // make this tile half its parents size and to the right of it
    tile->x = tile->parent->x + (tile->parent->w/2);
    tile->w = tile->parent->w/2;
    tile->y = tile->parent->y;
    tile->h = tile->parent->h;

    // make the parent half its size
    tile->parent->w /= 2;
  }

  for (Tile *tile = headtile; tile != NULL; tile = tile->next) {
    XMoveResizeWindow(dpy, tile->win,
        tile->x, tile->y, tile->w, tile->h);
    XMapWindow(dpy, tile->win);
  }
}

char KeysymToString(XKeyEvent *xkey) {
  return *XKeysymToString(XLookupKeysym(xkey, 0));
}

void setup_atoms(void) {
  UTF8_STRING = XInternAtom(dpy, "UTF8_STRING", False);

  wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
  wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);

  _NET_SUPPORTING_WM_CHECK = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  _NET_WM_NAME = XInternAtom(dpy, "_NET_WM_NAME", False);
  _NET_NUBER_OF_DESKTOPS = XInternAtom(dpy, "_NET_NUBER_OF_DESKTOPS", False);

  Window WmCheckWin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
  XChangeProperty(dpy, root, _NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
      PropModeReplace, (unsigned char*) &WmCheckWin, 1);
  XChangeProperty(dpy, WmCheckWin, _NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
      PropModeReplace, (unsigned char*) &WmCheckWin, 1);
  XChangeProperty(dpy, WmCheckWin, _NET_WM_NAME, UTF8_STRING, 8,
      PropModeReplace, (unsigned char*) WM_NAME, strlen(WM_NAME));
}

void setup(void) {
  headtile = NULL;

  for (int i = 0; i < LASTEvent; i++) {
    handler[i] = voidevent;
  }
  handler[KeyPress] = keypress;
  handler[MapRequest] = maprequest;
  handler[DestroyNotify] = destroynotify;
  handler[FocusIn] = focusin;
}

// this code needs to be worked on
// there is some posix stuff that dwm does that this code DOES NOT DO
void spawn(void) {
  if (fork() == 0) {
    char *args[]={"st",NULL}; execvp(args[0],args);
    exit(0); // kill child process
  }
}

void sendevent(Tile *tile, Atom proto) {
  int n;
  Atom *protocols;
  int exists = 0;
  XEvent ev;

  if (XGetWMProtocols(dpy, tile->win, &protocols, &n)) {
    while (!exists && n--)
      exists = protocols[n] == proto;
    XFree(protocols);
  }

  if (exists) {
    ev.type = ClientMessage;
    ev.xclient.type = ClientMessage;
    ev.xclient.window = tile->win;
    ev.xclient.message_type = wmatom[WMProtocols];
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = proto;
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(dpy, tile->win, False, NoEventMask, &ev);
  }
}

void setfocus(Tile *tile) {
  if (tile->win == root || tile == NULL)
    return;

  sendevent(tile, wmatom[WMTakeFocus]);
}

int xerror(Display *dpy, XErrorEvent *ee) {
  switch (ee->error_code) {
    case BadWindow:
      printf("Error BadWindow\n");
      return 0;
  }
  // from dwm
  fprintf(stderr, "%s: fatal error: request code=%d, error code=%d\n",
      WM_NAME, ee->request_code, ee->error_code);
  return xerrorxlib(dpy, ee);
}


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
  XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask | EnterWindowMask);

  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("q")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("a")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);

  printf("Default screen: %d\nScreen width: %d\nScreen height: %d\n", screen, screenw, screenh);

  setup_atoms();
  setup();
  xerrorxlib = XSetErrorHandler(xerror);

  for (;;) {
    XNextEvent(dpy, &ev);
    printf("event rec of type %d ", ev.type);

    handler[ev.type](&ev);
  }

  XCloseDisplay(dpy);
  printf("exiting with no errors\n");

  return 0;
}
