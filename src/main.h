#ifndef NEWWM_MAIN_H
#define NEWWM_MAIN_H

enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };
enum { NetActiveWindow, NetWMName, NetWMCheck, NetLast }; // ewmh atoms

Atom wmatom[WMLast];
Atom netatom[NetLast];

Display *dpy;
Window root;
int screenw, screenh;

typedef struct Client Client;
struct Client {
  Window win;
  Client *parent;
  Client *next;
  int x, y, w, h;
};

char KeysymToString(XKeyEvent *xkey);

int (*xerrorxlib)(Display *, XErrorEvent *);
int xerror(Display *dpy, XErrorEvent *ee);
void setupatoms(void);
void setup(void);
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

#endif
