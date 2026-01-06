#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"
#include "defs.h"


void printll(void) {
  printf("\nlinked list:\n");
  Client *c = headc;
  while (c) {
    printf("c: %-8lx, n: %-8lx, pr: %-8lx, pa: %-8lx, h: %d\n",
        c->win,
        (c->next != NULL ? c->next->win : 0L),
        (c->prev != NULL ? c->prev->win : 0L),
        (c->parent != NULL ? c->parent->win : 0L),
        (c == headc ? 1 : 0));
    c = c->next;
  }
}

void printerr(char *errstr) {
  fprintf(stderr, "%s: error: %s", WM_NAME, errstr);
}

void exitwm(int err) {
  XCloseDisplay(dpy);
  exit(err);
}

void voidevent(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(void event)\n");
#endif
}

char keysymtostring(XKeyEvent *xkey) {
  return *XKeysymToString(XLookupKeysym(xkey, 0));
}

void keypress(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(keypress)\n");
#endif

  XKeyEvent *xkey = &ev->xkey;
  char keysym = keysymtostring(xkey);
  for (int i = 0; i < LENGTH
  /*for (int i = 0; i < LENGTH(conf.keys); i++) {
    if (keysym == conf.keys[i]->keysym &&
        CLEANMASK(conf.keys[i]->mod) == CLEANMASK(xkey->state) &&
        conf.keys[i]->func)
      conf.keys[i]->func();
  }*/
  if (keysymtostring(xkey) == 'q' && xkey->state == Mod1Mask) {
    fprintf(stderr, "%s: exiting with no errors\n", WM_NAME);
    exitwm(0);
  } else if (keysymtostring(xkey) == 'a' && xkey->state == Mod1Mask) {
    spawn((char *[]){"st", NULL});
  } else if (keysymtostring(xkey) == 's' && xkey->state == Mod1Mask) {
    spawn((char *[]){"rofi", "-normal-window", "-show", "drun", NULL});
  } else if (keysymtostring(xkey) == 'x' && xkey->state == Mod1Mask) {
    kill(focused);
  }
}

void maprequest(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(maprequest)\n");
#endif
  XMapRequestEvent *mapreq = &ev->xmaprequest;
  Client *newc = (Client *)malloc(sizeof(Client));
  newc->win = mapreq->window;
  newc->parent = NULL;
  newc->next = NULL;
  newc->prev = NULL;

  XSelectInput(dpy, newc->win, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
  XSetWindowBorderWidth(dpy, newc->win, conf.bord_size);

  if (!headc) {
    headc = newc;
  } else {
    Client *c = headc;
    while (c->next)
      c = c->next;

    c->next = newc;
    newc->prev = c;
    newc->parent = focused;
  }
  masterstacktile();
  printll();
}

void unmapnotify(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(unmapnotify)\n");
#endif
  Window unmapwin = ev->xunmap.window;

  unmanage(unmapwin);
  masterstacktile();
}

void destroynotify(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(destroynotify)\n");
#endif
  Window destroywin = ev->xdestroywindow.window;

  unmanage(destroywin);
  masterstacktile();
}

void unmanage(Window deletewin) {
  if (!deletewin) {
    return;
  }

  Client *c = headc;
  while (c && c->win != deletewin) {
    c = c->next;
  }

  // try to somehow remove this and just use c
  Client *delc = c;

  if (!delc) {
    printerr("cannot find client/window\n");
    return;
  }
  //printf("destroy client: %x, win: %lx\n", delc, delc->win);

  printll();

  Client *lastchild = NULL;
  Client *firstchild = NULL;
  for (Client *cl = delc; cl; cl = cl->next) {
    if (cl->parent == delc) {
      if (firstchild)
        lastchild = cl;
      else
        firstchild = cl;
    }
  }

  if (firstchild && lastchild) {
    /*printf("last: %lx", lastchild->win);
    printf("first: %lx", firstchild->win);*/
    // detach
    lastchild->prev->next = lastchild->next;
    if (lastchild->next)
      lastchild->next->prev = lastchild->prev;

    // attach
    lastchild->prev = firstchild->prev;
    lastchild->prev->next = lastchild;
    firstchild->prev = lastchild;
    lastchild->next = firstchild;
  }

  //printll();

  if (delc == headc)
    headc = delc->next;
  if (delc->next)
    delc->next->prev = delc->prev;
  if (delc->prev)
    delc->prev->next = delc->next;

  while (c) {
    if (c->parent == delc) {
      c->parent = delc->parent;
      if (delc->parent == NULL)
        delc->parent = c;
    }
    c = c->next;
  }

  if (delc == focused && delc->parent) {
    focused = delc->parent;
    setfocus(focused);
  }

  free(delc);
#ifdef DEBUG
  printll();
#endif
}

void enternotify(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(enternotify)\n");
#endif
  if (ev->xcrossing.window == root)
    return;

  Client *c = headc;
  while (c->next && c->win != ev->xcrossing.window) {
    c = c->next;
  }
  focused = c;
  setfocus(c);
}

void focusin(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(focusin)\n");
#endif
  if (focused && ev->xfocus.window != focused->win) {
    setfocus(focused);
  }
}

int sendevent(Client *c, Atom proto) {
  int n;
  Atom *protocols;
  int exists = 0;
  XEvent ev;

  if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
    while (!exists && n--)
      exists = protocols[n] == proto;
    XFree(protocols);
  }

  if (exists) {
    ev.type = ClientMessage;
    //ev.xclient.type = ClientMessage;
    ev.xclient.window = c->win;
    ev.xclient.message_type = wmatom[WMProtocols];
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = proto;
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(dpy, c->win, False, NoEventMask, &ev);
  }

  return exists;
}

void setfocus(Client *c) {
  if (c->win == root || c == NULL)
    return;
  focused = c; // make sure focus is set
  XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
  XChangeProperty(dpy, root, netatom[NetActiveWindow],
      XA_WINDOW, 32, PropModeReplace,
      (unsigned char *) &(c->win), 1);
  sendevent(c, wmatom[WMTakeFocus]); // god know what this does
  
  updateborders();
}

void updateborders() {
  for (Client *c = headc; c; c = c->next) {
    XSetWindowBorder(dpy, c->win, (c == focused ? conf.bord_foc_col : conf.bord_nor_col));
  }
}

// "dwm tiling"
void masterstacktile(void) {
  for (Client *c = headc; c; c = c->next) {
    if (c == headc) {
      c->x = 0 + conf.hgaps; c->y = 0 + conf.vgaps;
      c->w = screenw - (2*conf.hgaps + 2*conf.bord_size); c->h = screenh - (2*conf.vgaps + 2*conf.bord_size);
      continue;
    }
    if (c->parent == NULL) {
      continue;
    }
    if (c->parent->w >= c->parent->h) {
      c->x = c->parent->x + (c->parent->w/2) + conf.hgaps/2 + conf.bord_size;
      c->w = c->parent->w/2 - conf.hgaps/2 - conf.bord_size;
      c->y = c->parent->y;
      c->h = c->parent->h;
      c->parent->w = c->parent->w/2 - conf.hgaps/2 - conf.bord_size;
    } else {
      c->x = c->parent->x;
      c->w = c->parent->w;
      c->y = c->parent->y + (c->parent->h/2) + conf.vgaps/2 + conf.bord_size;
      c->h = c->parent->h/2 - conf.vgaps/2 - conf.bord_size;
      c->parent->h = c->parent->h/2 - conf.vgaps/2 - conf.bord_size;
    }
  }

  for (Client *c = headc; c; c = c->next) {
    XMoveResizeWindow(dpy, c->win,
        c->x, c->y, c->w, c->h);
    XMapWindow(dpy, c->win);
  }

  updateborders();
}

void setup(void) {

  int screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  screenw = XDisplayWidth(dpy, screen);
  screenh = XDisplayHeight(dpy, screen);

  // https://tronche.com/gui/x/xlib/events/processing-overview.html
  XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask | EnterWindowMask);

  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("q")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("a")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("s")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("x")), Mod1Mask,
        root, True, GrabModeAsync, GrabModeAsync);

  headc = NULL;
  for (int i = 0; i < LASTEvent; i++)
    handler[i] = voidevent;
  handler[KeyPress] = keypress;
  handler[MapRequest] = maprequest;
  handler[DestroyNotify] = destroynotify;
  handler[UnmapNotify] = unmapnotify;
  handler[EnterNotify] = enternotify;
  handler[FocusIn] = focusin;

  //spawn((char *[]){"feh", "--bg-fill", "/home/ethan/dotfiles/images/wallpaper.png", NULL});
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
#ifdef NWM_DEBUG
  printf("spawning: %s\n", argv[0]);
#endif
  if (fork() == 0) {
    //char *args[]={program,NULL};
    setsid();
    execvp(argv[0], argv);
    exit(0); // kill child process
  }
}

void kill(Client *c) {
  if (!c)
    return;

  if (!sendevent(c, wmatom[WMDelete])) {
    XGrabServer(dpy);
    XSetErrorHandler(xerrordummy);
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, c->win);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XUngrabServer(dpy);
  }
}

int xerror(Display *dpy, XErrorEvent *ee) {
  // from dwm
  switch (ee->error_code) {
    case BadWindow:
      printerr("BadWindow\n");
      return 0;
  }
  fprintf(stderr, "%s: fatal error: request code=%d, error code=%d\n",
      WM_NAME, ee->request_code, ee->error_code);
  return xerrorxlib(dpy, ee);
}

int xerrordummy(Display *dpy, XErrorEvent *ee) {
  return 0;
}


int main() {
  XEvent ev;

  if (!(dpy = XOpenDisplay(NULL))) {
    printerr("failed to open display\n");
    exitwm(1);
  }
  //printf("Default screen: %d\nScreen width: %d\nScreen height: %d\n", screen, screenw, screenh);

  setup();
  setupatoms();
  xerrorxlib = XSetErrorHandler(xerror);

  for (;;) {
    XNextEvent(dpy, &ev);
#ifdef NWM_DEBUG
    printf("event rec of type %d ", ev.type);
#endif

    handler[ev.type](&ev);
  }

  printerr("exiting with no errors\n");
  exitwm(0);
  return 0;
}
