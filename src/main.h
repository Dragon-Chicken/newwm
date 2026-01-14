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
  bool dock;
};

typedef union Arg {
  int i;
  char **s;
} Arg;

typedef struct Key {
  int mod;
  KeySym keysym;
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

void printll(void);
void printerr(char *errstr);
char keysymtostring(XKeyEvent *xkey);
int getatomprop(Client *c, Atom prop, Atom *retatom);
int getcardprop(Client *c, Atom prop, int *strut, unsigned long strutlen);
bool intersect(int x1, int w1, int x2, int w2);

void (*handler[LASTEvent])(XEvent*);
void voidevent(XEvent *ev);
void keypress(XEvent *ev);
void maprequest(XEvent *ev);
void unmapnotify(XEvent *ev);
void destroynotify(XEvent *ev);
void enternotify(XEvent *ev);
void focusin(XEvent *ev);

int sendevent(Client *c, Atom proto);
void setfocus(Client *c);
void manage(Window w, XWindowAttributes *wa);
void unmanage(Window destroywin);
void masterstacktile(void);
void updateborders(void);
void setup(void);
void setupatoms(void);
void spawn(Arg *arg);
void focusswitch(Arg *arg);
void kill(Arg *arg);
void exitwm(Arg *arg);
int (*xerrorxlib)(Display *, XErrorEvent *);
int xerror(Display *dpy, XErrorEvent *ee);
int xerrordummy(Display *dpy, XErrorEvent *ee);

Client *headc;
Client *focused;

Config conf;

Display *dpy;
Window root;

int screenx, screeny;
int screenw, screenh;
// ScreenXOFF, ScreenYOFF...
int sxoff, syoff;
int swoff, shoff;

//#define NWM_DEBUG

#endif
