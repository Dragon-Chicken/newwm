#ifndef NWM_MAIN_H
#define NWM_MAIN_H

enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };
enum { NetActiveWindow, NetWMName, NetWMCheck, NetLast }; // ewmh atoms

Atom wmatom[WMLast];
Atom netatom[NetLast];

typedef struct Client Client;
struct Client {
  Window win;
  Client *parent;
  Client *next;
  Client *prev;
  int x, y, w, h;
};

typedef struct Config Config;
struct Config {
  int vgaps;
  int hgaps;
  int bord_size;
  long bord_foc_col;
  long bord_nor_col;
};

char KeysymToString(XKeyEvent *xkey);

int (*xerrorxlib)(Display *, XErrorEvent *);
int xerror(Display *dpy, XErrorEvent *ee);
void setup(void);
void setupatoms(void);
void masterstacktile(void);
void updateborders(void);
void spawn(char *argv[]);
void setfocus(Client *c);
void sendevent(Client *c, Atom proto);
void unmanage(Window destroywin);

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

Display *dpy;
Window root;
int screenw, screenh;

#endif
