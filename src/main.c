#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"
#include "defs.h"

void exitwm(int err) {
  XCloseDisplay(dpy);
  exit(err);
}

void voidevent(XEvent *ev) {
  printf("(void event)\n");
}

char keysymtostring(XKeyEvent *xkey) {
  return *XKeysymToString(XLookupKeysym(xkey, 0));
}

void keypress(XEvent *ev) {
  printf("(keypress)\n");

  XKeyEvent *xkey = &ev->xkey;
  if (keysymtostring(xkey) == 'q' && xkey->state == Mod1Mask) {
    fprintf(stderr, "exiting with no errors\n");
    exitwm(0);
  }
  if (keysymtostring(xkey) == 'a' && xkey->state == Mod1Mask) {
    spawn((char *[]){"st", NULL}); // what have I created....
  }
  if (keysymtostring(xkey) == 's' && xkey->state == Mod1Mask) {
    spawn((char *[]){"rofi", "-normal-window", "-show", "drun", NULL}); // what have I created....
  }
}

void maprequest(XEvent *ev) {
  printf("(maprequest)\n");
  XMapRequestEvent *mapreq = &ev->xmaprequest;
  Tile *newtile = (Tile *)malloc(sizeof(Tile));
  newtile->win = mapreq->window;
  newtile->parent = NULL;
  newtile->next = NULL;

  XSelectInput(dpy, newtile->win, EnterWindowMask | FocusChangeMask);
  XSetWindowBorderWidth(dpy, newtile->win, 4);

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

  if (!headtile) {
    headtile = newtile;
  } else {
    Tile *tile = headtile;
    while (tile->next)
      tile = tile->next;

    tile->next = newtile;
    newtile->parent = tile; // this code WILL NEED TO change depending on the layout
                            // better idea is to put the parent to the currently selected tile
  }
  masterstacktile();
}

void unmapnotify(XEvent *ev) {
  printf("(unmapnotify)\n");
  Window unmapwin = ev->xunmap.window;

  unmanage(unmapwin);
  masterstacktile();
}

void destroynotify(XEvent *ev) {
  printf("(destroynotify)\n");
  Window destroywin = ev->xdestroywindow.window;

  unmanage(destroywin);
  masterstacktile();

  // should prob put this in it's own function for debugging
  /*printf("linked list:\n");
  tile = headtile;
  while (tile) {
    printf("%lx\n", tile->win);
    tile = tile->next;
  }*/
}

void unmanage(Window deletewin) {
  /*
   * THIS DOES NOT HANDLE THE CASE WHERE A TILE WHO HAS CHILDREN GETS DELETED,
   * THOSE CHILDREN STILL THINK THIS TILE EXISTS
   * god knows if this ^ is fixed or not
   */

  Tile *prev = NULL;
  Tile *tile = headtile;
  while (tile && tile->win != deletewin) {
    prev = tile;
    tile = tile->next;
  }

  // try to somehow remove this and just use tile
  Tile *deletetile = tile;

  if (!deletetile) {
    printf("ERROR: cannot find tile/window\n");
    return;
  }

  printf("destroy tile: %x, win: %lx\n", deletetile, deletetile->win);
  if (deletetile == headtile)
    headtile = deletetile->next;
  if (prev)
    prev->next = deletetile->next;

  while (tile) {
    if (tile->parent == deletetile) {
      tile->parent = deletetile->parent;
      if (deletetile->parent == NULL)
        deletetile->parent = tile;
    }
    tile = tile->next;
  }

  if (deletetile == focused && deletetile->parent) {
    focused = deletetile->parent;
    printf("focusing win: %lx\n", focused->win);
    XSetInputFocus(dpy, focused->win, RevertToPointerRoot, CurrentTime);
  }

  free(deletetile);
}

void enternotify(XEvent *ev) {
  printf("(enternotify)\n");
  if (ev->xcrossing.window == root)
    return;

  Tile *tile = headtile;
  while (tile->next && tile->win != ev->xcrossing.window) {
    tile = tile->next;
  }
  printf("win: %lx\n", tile->win);
  focused = tile;
  setfocus(tile);
}

void focusin(XEvent *ev) {
  printf("(focusin)\n");
  if (focused && ev->xfocus.window != focused->win) {
    setfocus(focused);
  }
}

void setfocus(Tile *tile) {
  if (tile->win == root || tile == NULL)
    return;
  focused = tile; // make sure focus is set
  //sendevent(tile, wmatom[WMTakeFocus]); // god know what this does
  printf("focusing win: %lx\n", tile->win);
  XSetInputFocus(dpy, tile->win, RevertToPointerRoot, CurrentTime);
  XChangeProperty(dpy, root, netatom[NetActiveWindow],
      XA_WINDOW, 32, PropModeReplace,
      (unsigned char *) &(tile->win), 1);
  
  updateborders();
}


void updateborders() {
  for (Tile *tile = headtile; tile; tile = tile->next) {
    XSetWindowBorder(dpy, tile->win, (tile == focused ? 0xffff0000L : 0xff0000ffL));
  }
}

// "dwm tiling"
void masterstacktile(void) {
  for (Tile *tile = headtile; tile; tile = tile->next) {
    int vgaps = GAPS;
    int hgaps = GAPS;
    if (tile == headtile) {
      tile->x = 0 + hgaps; tile->y = 0 + vgaps;
      tile->w = screenw - (2*hgaps); tile->h = screenh - (2*vgaps);
      continue;
    }
    if (tile->parent == NULL) {
      continue;
    }
    tile->x = tile->parent->x + (tile->parent->w/2) + hgaps/2;
    tile->w = tile->parent->w/2 - hgaps/2;
    tile->y = tile->parent->y;
    tile->h = tile->parent->h;
    tile->parent->w = tile->parent->w/2 - hgaps/2;
  }

  for (Tile *tile = headtile; tile; tile = tile->next) {
    XMoveResizeWindow(dpy, tile->win,
        tile->x, tile->y, tile->w, tile->h);
    XMapWindow(dpy, tile->win);
  }

  updateborders();
}

void setup(void) {
  headtile = NULL;
  for (int i = 0; i < LASTEvent; i++)
    handler[i] = voidevent;
  handler[KeyPress] = keypress;
  handler[MapRequest] = maprequest;
  handler[DestroyNotify] = destroynotify;
  handler[UnmapNotify] = unmapnotify;
  handler[EnterNotify] = enternotify;
  handler[FocusIn] = focusin;
  
  spawn((char *[]){"feh", "--bg-fill", "/home/ethan/dotfiles/images/wallpaper.png", NULL});
}

void setupatoms(void) {
  Atom utf8string;
  utf8string = XInternAtom(dpy, "UTF8_STRING", False);

  wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
  wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);

  netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);

  Window WmCheckWin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
  XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
      PropModeReplace, (unsigned char*) &WmCheckWin, 1);
  XChangeProperty(dpy, WmCheckWin, netatom[NetWMCheck], XA_WINDOW, 32,
      PropModeReplace, (unsigned char*) &WmCheckWin, 1);
  XChangeProperty(dpy, WmCheckWin, netatom[NetWMName], utf8string, 8,
      PropModeReplace, (unsigned char*) WM_NAME, strlen(WM_NAME));
}

// this code needs to be worked on
// there is some posix stuff that dwm does that this code DOES NOT DO
void spawn(char *argv[]) {
  printf("\n%s\n", argv[0]);
  if (fork() == 0) {
    //char *args[]={program,NULL};
    setsid();
    execvp(argv[0], argv);
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

int xerror(Display *dpy, XErrorEvent *ee) {
  // from dwm
  switch (ee->error_code) {
    case BadWindow:
      printf("ERROR: BadWindow\n");
      return 0;
  }
  fprintf(stderr, "%s: fatal error: request code=%d, error code=%d\n",
      WM_NAME, ee->request_code, ee->error_code);
  return xerrorxlib(dpy, ee);
}


int main() {
  XEvent ev;

  if (!(dpy = XOpenDisplay(NULL))) {
    fprintf(stderr, "failed to open display\n");
    exitwm(1);
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
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("s")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);

  printf("Default screen: %d\nScreen width: %d\nScreen height: %d\n", screen, screenw, screenh);

  setupatoms();
  setup();
  xerrorxlib = XSetErrorHandler(xerror);

  for (;;) {
    XNextEvent(dpy, &ev);
    printf("event rec of type %d ", ev.type);

    handler[ev.type](&ev);
  }

  fprintf(stderr, "exiting with no errors\n");
  exitwm(0);
  return 0;
}
