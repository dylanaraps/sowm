#include <xcb/xcb.h>

extern xcb_connection_t *dpy;
extern xcb_screen_t *scr;

struct desktop {
    xcb_window_t *windows;
    int num;
};

extern int current_desktop;
