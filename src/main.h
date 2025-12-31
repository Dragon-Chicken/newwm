#ifndef NEWWM_MAIN_H
#define NEWWM_MAIN_H

enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };
enum { NetActiveWindow, NetWMName, NetWMCheck, NetLast }; // ewmh atoms

Atom wmatom[WMLast];
Atom netatom[NetLast];

Display *dpy;
Window root;
int screenw, screenh;

typedef struct Tile Tile;
struct Tile {
  Window win;
  Tile *parent;
  Tile *next;
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
void setfocus(Tile *tile);
void sendevent(Tile *tile, Atom proto);
void unmanage(Window destroywin);

void (*handler[LASTEvent])(XEvent*);
void voidevent(XEvent *ev);
void keypress(XEvent *ev);
void maprequest(XEvent *ev);
void destroynotify(XEvent *ev);
void unmapnotify(XEvent *ev);
void focusin(XEvent *ev);
void enternotify(XEvent *ev);

Tile *headtile;
Tile *focused;

#endif
