// sowm - An itsy bitsy floating window manager.

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

typedef union {
    const char** com;
    const int i;
    const Window w;
} Arg;

struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const Arg arg);
    const Arg arg;
};

typedef struct client {
    struct client *next, *prev;
    int f, wx, wy;
    unsigned int w, ww, wh;
} client;

static void button_press(XEvent *e);
static void button_release();
static void configure_request(XEvent *e);
static void key_press(XEvent *e);
static void map_request(XEvent *e);
static void notify_destroy(XEvent *e);
static void notify_enter(XEvent *e);
static void notify_motion(XEvent *e);
static void run(const Arg arg);
static void win_add(Window w);
static void win_center(const Arg arg);
static void win_del(Window w);
static void win_fs();
static void win_kill();
static void win_next();
static void win_to_ws(const Arg arg);
static void ws_go(const Arg arg);
static int  xerror() { return 0;}

static client       *list = {0}, *ws_list[10] = {0}, *cur = {0};
static int          ws = 1, sw, sh, wx, wy;
static unsigned int ww, wh;

static Display      *d;
static Window       root;
static XButtonEvent mouse;

static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
    [ConfigureRequest] = configure_request,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [DestroyNotify]    = notify_destroy,
    [EnterNotify]      = notify_enter,
    [MotionNotify]     = notify_motion
};

#include "config.h"

#define win          (client *c=list;c;c=c->next)
#define ws_save(W)   ws_list[W] = list
#define ws_sel(W)    list = ws_list[ws = W]

#define win_size(W, gx, gy, gw, gh) \
    XGetGeometry(d, W, &(Window){0}, gx, gy, gw, gh, \
                 &(unsigned int){0}, &(unsigned int){0})

void win_focus(Window w) {
    XSetInputFocus(d, w, RevertToParent, CurrentTime);

    for win if (c->w == w) cur = c;
}

void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);

    if (list) win_focus(list->w);
}

void notify_enter(XEvent *e) {
    while(XCheckTypedEvent(d, EnterNotify, e));

    win_focus(e->xcrossing.window);
}

void notify_motion(XEvent *e) {
    if (mouse.subwindow == 0) return;

    int xd = e->xbutton.x_root - mouse.x_root;
    int yd = e->xbutton.y_root - mouse.y_root;

    while(XCheckTypedEvent(d, MotionNotify, e));

    XMoveResizeWindow(d, mouse.subwindow,
        wx + (mouse.button == 1 ? xd : 0),
        wy + (mouse.button == 1 ? yd : 0),
        ww + (mouse.button == 3 ? xd : 0),
        wh + (mouse.button == 3 ? yd : 0));
}

void key_press(XEvent *e) {
    KeySym keysym = XKeycodeToKeysym(d, e->xkey.keycode, 0);

    for (unsigned int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        if (keys[i].keysym == keysym && keys[i].mod == e->xkey.state)
            keys[i].function(keys[i].arg);
}

void button_press(XEvent *e) {
    if (e->xbutton.subwindow == 0) return;

    win_size(e->xbutton.subwindow, &wx, &wy, &ww, &wh);
    XRaiseWindow(d, e->xbutton.subwindow);
    mouse = e->xbutton;
}

void button_release() {
    for win if (c->w == mouse.subwindow) c->f = 0;

    mouse.subwindow = 0;
}

void win_add(Window w) {
    client *c, *t;

    if (!(c = (client *)calloc(1, sizeof(client))))
        exit(1);

    if (!list) {
        c->next = c->prev = 0;
        c->w    = w;
        list    = c;

    } else {
        for (t=list;t->next;t=t->next);

        c->next = 0;
        c->prev = t;
        c->w    = w;
        t->next = c;
    }

    ws_save(ws);
}

void win_del(Window w) {
    for win if (c->w == w) {
        if (!c->prev && !c->next) {
            free(list);
            list = 0;
            ws_save(ws);
            return;
        }

        if (!c->prev) {
            list = c->next;
            c->next->prev = 0;

        } else if (!c->next) {
            c->prev->next = 0;

        } else {
            c->prev->next = c->next;
            c->next->prev = c->prev;
        }

        free(c);
        ws_save(ws);
        return;
    }
}

void win_kill() {
    if (cur) XKillClient(d, cur->w);
}

void win_center(const Arg arg) {
    if (!cur) return;

    Window w = arg.w ? arg.w : cur->w;

    win_size(w, &(int){0}, &(int){0}, &ww, &wh);

    XMoveWindow(d, w, (sw - ww) / 2, (sh - wh) / 2);
}

void win_fs() {
    if (!cur) return;

    if ((cur->f = cur->f == 0 ? 1 : 0)) {
        win_size(cur->w, &cur->wx, &cur->wy, &cur->ww, &cur->wh);
        XMoveResizeWindow(d, cur->w, 0, 0, sw, sh);

    } else
        XMoveResizeWindow(d, cur->w, cur->wx, cur->wy, cur->ww, cur->wh);
}

void win_to_ws(const Arg arg) {
    int tmp = ws;

    if (arg.i == tmp) return;

    ws_sel(arg.i);
    win_add(cur->w);
    ws_save(arg.i);

    ws_sel(tmp);
    win_del(cur->w);
    XUnmapWindow(d, cur->w);
    ws_save(tmp);

    if (list) win_focus(list->w);
}

void win_next() {
    if (!list) return;

    client *c = cur->next ? cur->next : list;

    win_focus(c->w);
    XRaiseWindow(d, c->w);
}

void ws_go(const Arg arg) {
    int tmp = ws;

    if (arg.i == ws) return;

    ws_save(ws);
    ws_sel(arg.i);

    if (list) for win XMapWindow(d, c->w);

    ws_sel(tmp);

    if (list) for win XUnmapWindow(d, c->w);

    ws_sel(arg.i);

    if (list) win_focus(list->w);
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;

    XConfigureWindow(d, ev->window, ev->value_mask, &(XWindowChanges) {
        .x          = ev->x,
        .y          = ev->y,
        .width      = ev->width,
        .height     = ev->height,
        .sibling    = ev->above,
        .stack_mode = ev->detail
    });
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;

    XSelectInput(d, w, StructureNotifyMask|EnterWindowMask);

    win_size(w, &wx, &wy, &ww, &wh);
    win_add(w);
    win_focus(w);

    if (wx == 0 && wy == 0) win_center((Arg){.i = w});

    XMapWindow(d, w);
}

void run(const Arg arg) {
    if (fork()) return;
    if (d) close(ConnectionNumber(d));

    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

int main(void) {
    XEvent ev;

    if (!(d = XOpenDisplay(0))) exit(1);

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);

    int s = DefaultScreen(d);
    root  = RootWindow(d, s);
    sw    = XDisplayWidth(d, s);
    sh    = XDisplayHeight(d, s);

    XSelectInput(d,  root, SubstructureRedirectMask);
    XDefineCursor(d, root, XCreateFontCursor(d, 68));

    for (unsigned int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        XGrabKey(d, XKeysymToKeycode(d, keys[i].keysym), keys[i].mod,
                 root, True, GrabModeAsync, GrabModeAsync);

    for (int i=1; i<4; i+=2)
        XGrabButton(d, i, MOD, root, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
            GrabModeAsync, GrabModeAsync, 0, 0);

    while (1 && !XNextEvent(d, &ev))
        if (events[ev.type]) events[ev.type](&ev);
}
