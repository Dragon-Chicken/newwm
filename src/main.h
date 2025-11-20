#ifndef NEWWM_MAIN_H
#define NEWWM_MAIN_H

Display *dpy;
Window root;
int screenw, screenh;

Atom _NET_SUPPORTING_WM_CHECK;
Atom _NET_NUBER_OF_DESKTOPS;
Atom _NET_WM_NAME;
Atom UTF8_STRING;

// TEMP
// each virtual desktop needs it's own mapped count
int totalmapped;

char KeysymToString(XKeyEvent *xkey);

void setup_atoms(void);
void setup(void);
void tile(void);

void (*handler[LASTEvent])(XEvent*);
void keypress(XEvent *ev);
void maprequest(XEvent *ev);
void destroynotify(XEvent *ev);
void void_event(XEvent *ev);

typedef struct Tile Tile;

struct Tile {
  // window, parent tile
  Window win;
  Tile *parent;
  Tile *next;
};

Tile *headtile;

#endif
