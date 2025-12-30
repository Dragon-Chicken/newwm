#ifndef NEWWM_MAIN_H
#define NEWWM_MAIN_H

Atom _NET_SUPPORTING_WM_CHECK;
Atom _NET_NUBER_OF_DESKTOPS;
Atom _NET_WM_NAME;
Atom UTF8_STRING;

enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };

Atom wmatom[WMLast];

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
void setup_atoms(void);
void setup(void);
void master_stack_tile(void);
void spawn(void);
void setfocus(Tile *tile);
void sendevent(Tile *tile, Atom proto);

void (*handler[LASTEvent])(XEvent*);
void voidevent(XEvent *ev);
void keypress(XEvent *ev);
void maprequest(XEvent *ev);
void destroynotify(XEvent *ev);
void focusin(XEvent *ev);
void enternotify(XEvent *ev);

Tile *headtile;
Tile *focused;

#endif
