#ifndef NWM_MAIN_H
#define NWM_MAIN_H

#define WM_NAME "nwm"

#define LENGTH(X) (int)(sizeof(X) / sizeof(X[0]))
#define CLEANMASK(mask) (mask & ~(LockMask) & (Mod1Mask))

enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; // wmatom
enum { NetSupported, NetWMName, NetActiveWindow, NetWMCheck,
  NetWMStrutPartial,
  NetWMWindowType, NetWMWindowTypeNormal, NetWMWindowTypeDock, NetWMWindowTypePopup,
  NetLast }; // netatom

Atom wmatom[WMLast];
Atom netatom[NetLast];

typedef struct Client Client;
struct Client {
  Window win;
  Client *parent;
  Client *next;
  Client *prev;
  int x, y, w, h;
  bool floating;
  bool manage;
  bool bar;
};

typedef union Arg {
  int i;
  char **s;
} Arg;

typedef struct Key {
  int mod;
  int keysym;
  void (*func)(Arg *);
  Arg args;
} Key;

typedef struct Config {
  int vgaps;
  int hgaps;
  int bord_size;
  long bord_foc_col;
  long bord_nor_col;
  int keyslen;
  Key *keys;
} Config;

char keysymtostring(XKeyEvent *xkey);

int (*xerrorxlib)(Display *, XErrorEvent *);
int xerror(Display *dpy, XErrorEvent *ee);
int xerrordummy(Display *dpy, XErrorEvent *ee);
void setup(void);
void setupatoms(void);
void masterstacktile(void);
void updateborders(void);
void spawn(Arg *arg);
void kill(Arg *arg);
void setfocus(Client *c);
int sendevent(Client *c, Atom proto);
void unmanage(Window destroywin);
void exitwm(Arg *arg);

void (*handler[LASTEvent])(XEvent*);
void voidevent(XEvent *ev);
void keypress(XEvent *ev);
void maprequest(XEvent *ev);
void destroynotify(XEvent *ev);
void unmapnotify(XEvent *ev);
void focusin(XEvent *ev);
void enternotify(XEvent *ev);

Client *headc;
Client *focused;

Config conf;

Display *dpy;
Window root;
int screenx, screeny;
int screenw, screenh;

//#define NWM_DEBUG

#endif
