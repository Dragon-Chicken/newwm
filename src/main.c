#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"


// helper
void printll(void) {
  printf("\nlinked list:\n");
  Client *c = headc;
  while (c) {
    printf("c: %-8lx, n: %-8lx, pr: %-8lx, pa: %-8lx, h: %d, m: %d\n",
        c->win,
        (c->next != NULL ? c->next->win : 0L),
        (c->prev != NULL ? c->prev->win : 0L),
        (c->parent != NULL ? c->parent->win : 0L),
        (c == headc ? 1 : 0),
        c->manage);
    c = c->next;
  }
}

void printerr(char *errstr) {
  fprintf(stderr, "%s: error: %s", WM_NAME, errstr);
}

char keysymtostring(XKeyEvent *xkey) {
  return *XKeysymToString(XLookupKeysym(xkey, 0));
}

int getatomprop(Client *c, Atom prop, Atom *retatom) {
  int di;
  unsigned long dl;
  unsigned char *p = NULL;
  Atom da;
  if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof(*retatom), False, XA_ATOM,
      &da, &di, &dl, &dl, &p) == Success && p) {
    *retatom = *(Atom *)p;
    XFree(p);
    return 1;
  }
  return 0;
}

int getcardprop(Client *c, Atom prop, int *strut, unsigned long strutlen) {
  int di;
  unsigned long ni;
  unsigned long dl;
  unsigned char *p = NULL;
  Atom da;
  if (XGetWindowProperty(dpy, c->win, prop, 0L, strutlen, False, XA_CARDINAL,
      &da, &di, &ni, &dl, &p) == Success && p) {
    for (unsigned long i = 0; ni == strutlen && i <= ni; i++) {
      strut[i] = ((long *)p)[i];
    }
    return 1;
  }
  return 0;
}

bool intersect(int x1, int w1, int x2, int w2) {
  return (x1<=x2 && (x1+w1>=x2 || x1+w1>=x2+w2)) ||
         (x2<=x1 && (x2+w2>=x1 || x2+w2>=x1+w1));
}



// event handler
void voidevent(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(void event)\n");
#endif
}

void keypress(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(keypress)\n");
#endif

  XKeyEvent *xkey = &ev->xkey;
  KeySym keysym = XLookupKeysym(xkey, 0);
  for (int i = 0; i < conf.keyslen; i++) {
    if (keysym == (conf.keys)[i].keysym &&
        CLEANMASK(conf.keys[i].mod) == CLEANMASK(xkey->state) &&
        conf.keys[i].func) {
      conf.keys[i].func(&conf.keys[i].args);
    }
  }

  // only here to make sure it's always possible to exit wm
  if (keysymtostring(xkey) == 'q' && xkey->state == Mod1Mask) {
    fprintf(stderr, "%s: exiting with no errors\n", WM_NAME);
    exitwm(0);
  }
}

void maprequest(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(maprequest)\n");
#endif
  XMapRequestEvent *mapreq = &ev->xmaprequest;
  XWindowAttributes wa;
  if (!XGetWindowAttributes(dpy, mapreq->window, &wa) || wa.override_redirect) {
    return;
  }
  manage(mapreq->window, &wa);
}

void unmapnotify(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(unmapnotify)\n");
#endif
  Window unmapwin = ev->xunmap.window;

  unmanage(unmapwin);
}

void destroynotify(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(destroynotify)\n");
#endif
  Window destroywin = ev->xdestroywindow.window;

  unmanage(destroywin);
}

void enternotify(XEvent *ev) {
#ifdef NWM_DEBUG
  printf("(enternotify)\n");
#endif
  if (ev->xcrossing.window == root)
    return;

  Client *c = headc;
  while (c->next && c->win != ev->xcrossing.window)
    c = c->next;
  if (!c->manage)
    return;
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


// others
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
  XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
  focused = c; // make sure focus is set
  XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
  XChangeProperty(dpy, root, netatom[NetActiveWindow],
      XA_WINDOW, 32, PropModeReplace,
      (unsigned char *) &(c->win), 1);
  sendevent(c, wmatom[WMTakeFocus]); // god know what this does
  
  updateborders();
}

void manage(Window w, XWindowAttributes *wa) {
  Client *newc = (Client *)malloc(sizeof(Client));
  newc->win = w;
  newc->parent = NULL;
  newc->next = NULL;
  newc->prev = NULL;
  newc->floating = false;
  newc->manage = true;
  newc->dock = false;

  newc->x = wa->x;
  newc->y = wa->y;
  newc->w = wa->width;
  newc->h = wa->height;

  Atom wtype;
  if (getatomprop(newc, netatom[NetWMWindowType], &wtype)) {
    if (wtype == netatom[NetWMWindowTypeDock]) {
      int strut[12];
      if (getcardprop(newc, netatom[NetWMStrutPartial], strut, LENGTH(strut))) {
        // left edge
        sxoff += strut[0];
        swoff -= sxoff;
        // right edge
        swoff -= strut[1];
        // top edge
        syoff += strut[2];
        shoff -= syoff;
        // bottom edge
        shoff -= strut[3];
      }

      XMapWindow(dpy, newc->win);
      masterstacktile();
      free(newc);
      return;
    }
  }

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

  //printf("destroy client: %x, win: %lx, manage: %d\n", delc, delc->win, delc->manage);

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

  masterstacktile();
}

// yeah this name is never changing even though it's using something like bsp tiling xd
void masterstacktile(void) {
  for (Client *c = headc; c; c = c->next) {
    if (!c->manage || c->floating || c->dock) {
      continue;
    }
    if (c == headc) {
      c->x = (screenx + sxoff) + conf.hgaps;
      c->y = (screeny + syoff) + conf.vgaps;
      c->w = (screenw + swoff) - (2*conf.hgaps + 2*conf.bord_size);
      c->h = (screenh + shoff) - (2*conf.vgaps + 2*conf.bord_size);
      continue;
    }
    if (!c->parent) {
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
    if (c->manage && !c->dock) {
      XMoveResizeWindow(dpy, c->win,
          c->x, c->y, c->w, c->h);
    }
    XMapWindow(dpy, c->win);
  }

  updateborders();
}

void updateborders() {
  for (Client *c = headc; c; c = c->next) {
    if (c->manage)
      XSetWindowBorder(dpy, c->win, (c == focused ? conf.bord_foc_col : conf.bord_nor_col));
  }
}

void setup(void) {

  int screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  screenw = XDisplayWidth(dpy, screen);
  screenh = XDisplayHeight(dpy, screen);
  // for docks/bars
  sxoff = 0;
  syoff = 0;

  // https://tronche.com/gui/x/xlib/events/processing-overview.html
  XSelectInput(dpy, root, SubstructureRedirectMask|SubstructureNotifyMask|FocusChangeMask|EnterWindowMask|PropertyChangeMask);

  headc = NULL;
  for (int i = 0; i < LASTEvent; i++)
    handler[i] = voidevent;
  handler[KeyPress] = keypress;
  handler[MapRequest] = maprequest;
  handler[DestroyNotify] = destroynotify;
  handler[UnmapNotify] = unmapnotify;
  handler[EnterNotify] = enternotify;
  handler[FocusIn] = focusin;

  conf = (Config){
    .vgaps = 8,
    .hgaps = 8,
    .bord_size = 4,
    .bord_foc_col = 0xffc4a7e7L,
    .bord_nor_col = 0xff26233aL,
    .keyslen = 9,
  };

  // temp
  conf.keys = malloc(sizeof(Key) * conf.keyslen);

  conf.keys[0] = (Key){Mod1Mask, XStringToKeysym("q"), exitwm, {0}};

  char **arg = malloc(sizeof(char *) * 2);
  arg[0] = "st";
  arg[1] = NULL;
  conf.keys[1] = (Key){Mod1Mask, XStringToKeysym("a"), spawn, {.s = arg}};

  conf.keys[2] = (Key){Mod1Mask, XStringToKeysym("x"), kill, {0}};

  arg = malloc(sizeof(char *) * 5);
  arg[0] = "rofi";
  arg[1] = "-normal-window";
  arg[2] = "-show";
  arg[3] = "drun";
  arg[4] = NULL;
  conf.keys[3] = (Key){Mod1Mask, XStringToKeysym("s"), spawn, {.s = arg}};

  arg = malloc(sizeof(char *) * 2);
  arg[0] = "polybar";
  arg[1] = NULL;
  conf.keys[4] = (Key){Mod1Mask, XStringToKeysym("d"), spawn, {.s = arg}};

  conf.keys[5] = (Key){Mod1Mask, XStringToKeysym("h"), focusswitch, {0}};
  conf.keys[6] = (Key){Mod1Mask, XStringToKeysym("l"), focusswitch, {1}};
  conf.keys[7] = (Key){Mod1Mask, XStringToKeysym("k"), focusswitch, {2}};
  conf.keys[8] = (Key){Mod1Mask, XStringToKeysym("j"), focusswitch, {3}};

  // grab input
  for (int i = 0; i < conf.keyslen; i++) {
    XGrabKey(dpy, XKeysymToKeycode(dpy, conf.keys[i].keysym), conf.keys[i].mod,
          root, True, GrabModeAsync, GrabModeAsync);
  }

  // startup script
  /*arg = malloc(sizeof(char *) * 2);
  arg[0] = "/home/ethan/neowm/startup";
  arg[1] = NULL;
  spawn(&(Arg){.s = arg}); // temp*/
}

void setupatoms(void) {
  Atom utf8string;
  utf8string = XInternAtom(dpy, "UTF8_STRING", False);

  wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
  wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);

  netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
  netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
  netatom[NetWMStrutPartial] = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);

  netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  netatom[NetWMWindowTypeNormal] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);
  netatom[NetWMWindowTypeDock] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
  netatom[NetWMWindowTypePopup] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);

  Window WmCheckWin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
  XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
      PropModeReplace, (unsigned char*) &WmCheckWin, 1);
  XChangeProperty(dpy, WmCheckWin, netatom[NetWMCheck], XA_WINDOW, 32,
      PropModeReplace, (unsigned char*) &WmCheckWin, 1);
  XChangeProperty(dpy, WmCheckWin, netatom[NetWMName], utf8string, 8,
      PropModeReplace, (unsigned char*) WM_NAME, strlen(WM_NAME));

  XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
      PropModeReplace, (unsigned char *) netatom, NetLast);
}

// this code needs to be worked on
// there is some posix stuff that dwm does that this code DOES NOT DO
// god know if this is fixed ^
void spawn(Arg *arg) {
#ifdef NWM_DEBUG
  printf("spawning: %s\n", arg->s[0]);
#endif
  if (fork() == 0) {
    setsid();
    execvp(arg->s[0], arg->s);
    exit(0); // kill child process
  }
}

// 0 left, 1 right, 2 up, 3 down
void focusswitch(Arg *args) {
  Client *closest = NULL;
  // need to set to something very high xd
  int closx = screenw*2;
  int closy = screenh*2;
  int focmidx = (focused->x*2 + focused->w)/2;
  int focmidy = (focused->y*2 + focused->h)/2;

  Client *c = headc;
  while (c) {
    if (c == focused) {
      c = c->next;
      continue;
    }

    int cmidx = (c->x*2+c->w)/2;
    int cmidy = (c->y*2+c->h)/2;

    if ((abs(cmidx-focmidx) + abs(cmidy-focmidy) <= closx + closy) &&
        ((args->i == 0 && cmidx-focmidx <= 0) ||
         (args->i == 1 && cmidx-focmidx >= 0) ||
         (args->i == 2 && cmidy-focmidy <= 0) ||
         (args->i == 3 && cmidy-focmidy >= 0)) &&
        ((args->i <= 1 && intersect(c->y, c->h, focused->y, focused->h)) ||
         (args->i >= 2 && intersect(c->x, c->w, focused->x, focused->w)))) {
      closx = abs(cmidx-focmidx);
      closy = abs(cmidy-focmidy);
      closest = c;
    }

    c = c->next;
  }

  if (closest) {
    focused = closest;
    setfocus(closest);
  }
}

void kill(Arg *arg) {
  if (!focused)
    return;

  if (!sendevent(focused, wmatom[WMDelete])) {
    XGrabServer(dpy);
    XSetErrorHandler(xerrordummy);
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, focused->win);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XUngrabServer(dpy);
  }
}

void exitwm(Arg *arg) {
  XCloseDisplay(dpy);
  exit(arg->i);
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
    exitwm(0);
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
