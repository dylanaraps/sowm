// sowm - An itsy bitsy floating window manager.

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define WIN    (c=list;c;c=c->next)
#define FOC(W) XSetInputFocus(d, W, RevertToParent, CurrentTime);

typedef union {
    const char** com;
    const int i;
} Arg;

struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const Arg arg);
    const Arg arg;
};

typedef struct client client;
struct client{
    client *next, *prev;
    Window w;
    XWindowAttributes a;
    int f;
};

typedef struct desktop desktop;
struct desktop{client *list;};

static void button_press(XEvent *e);
static void button_release();
static void configure_request(XEvent *e);
static void key_grab();
static void key_press(XEvent *e);
static void map_request(XEvent *e);
static void notify_destroy(XEvent *e);
static void notify_enter(XEvent *e);
static void notify_motion(XEvent *e);
static void run(const Arg arg);
static void win_add(Window w);
static void win_center(Window w);
static void win_center_current();
static void win_del(Window w);
static void win_fs(Window w);
static void win_fs_current();
static void win_kill();
static void win_next();
static void win_to_ws(const Arg arg);
static void ws_go(const Arg arg);
static void ws_save(int i);
static void ws_sel(int i);

static client  *list = {0};
static desktop ws_list[10];
static int     ws = 1, sh, sw, s, j;

static Display           *d;
static Window            root, cur;
static XButtonEvent      mouse;
static XWindowAttributes attr;

#include "config.h"

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

/*
   'sowm' doesn't keep track of the currently focused window
   and instead grabs window under the cursor when needed.

   This is a super lazy way of handling current focus, however
   it aligns perfectly with mouse-follows-focus.

   Logic below will select a real window if this function
   returns the 'root' window.

   This function returns the current window while at the same
   time defining a global variable to contain its value. This
   allows for stupidily simple usage.

   Example: if (win_current() != root) XKillClient(d, cur);

   The value can be used as function output and then
   the same value can be used as a variable directly afterwards.
*/
Window win_current() {
    XGetInputFocus(d, &cur, &j);
    return cur;
}

/*
   When a window is destroyed it is first removed from the
   current desktop's window list and finally focus is shifted.

   Focus goes to the window under the cursor if it is *not*
   the root window. If it is the root window, focus goes to
   the first window in the desktop.
*/
void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);

    if (list) FOC(win_current() == root ? list->w : cur);
}

/*
   When the mouse enters or leaves a window this function
   handles which window shall be focused next.

   The while loop firstly compresses all 'EnterNotify'
   events down to only the latest which is an optimization
   when focus changes very quickly (e.g a desktop focus).

   There's no use in computing each and every event as we
   only really care about the newest one.

   Focus is only then changed if the mouse has entered a
   window which is *not* the root window.
*/
void notify_enter(XEvent *e) {
    while(XCheckTypedEvent(d, EnterNotify, e));

    if (e->xcrossing.window != root) FOC(e->xcrossing.window)
}

/*
   When the mouse is moved and the paired modifier is
   pressed this function handles a window move or a window
   resize.

   'mouse' is defined on a modifier+mouse press and then
   discarded on a modifier+mouse release.

   The while loop firstly compresses all 'MotionNotify'
   events down to only the latest which is an optimization
   when motion happens very quickly.

   There's no use in computing each and every event as we
   only really care about the newest one.

   The window is then moved or resized and finally its
   fullscreen value is reset to '0' (False).
*/
void notify_motion(XEvent *e) {
    client *c;

    if (mouse.subwindow != None) {
        int xd = e->xbutton.x_root - mouse.x_root;
        int yd = e->xbutton.y_root - mouse.y_root;

        while(XCheckTypedEvent(d, MotionNotify, e));

        XMoveResizeWindow(d, mouse.subwindow,
            attr.x      + (mouse.button==1 ? xd : 0),
            attr.y      + (mouse.button==1 ? yd : 0),
            attr.width  + (mouse.button==3 ? xd : 0),
            attr.height + (mouse.button==3 ? yd : 0));

        for WIN if (c->w == mouse.subwindow) c->f = 0;
    }
}

void key_grab() {
    KeyCode code;

    for (int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        if ((code = XKeysymToKeycode(d, keys[i].keysym)))
            XGrabKey(d, code, keys[i].mod, root,
                     True, GrabModeAsync, GrabModeAsync);
}

void key_press(XEvent *e) {
    KeySym keysym = XKeycodeToKeysym(d, e->xkey.keycode, 0);

    for (int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        if (keys[i].keysym == keysym && keys[i].mod == e->xkey.state)
            keys[i].function(keys[i].arg);
}

void button_press(XEvent *e) {
    if (e->xbutton.subwindow == None) return;

    XGetWindowAttributes(d, e->xbutton.subwindow, &attr);
    XRaiseWindow(d, e->xbutton.subwindow);
    mouse = e->xbutton;
}

void button_release() {
    mouse.subwindow = None;
}

void win_add(Window w) {
    client *c, *t;

    if (!(c = (client *)calloc(1, sizeof(client))))
        exit(1);

    if (!list) {
        c->next = 0;
        c->prev = 0;
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
    client *c;

    for WIN if (c->w == w) {
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
    if (win_current() != root) XKillClient(d, cur);
}

void win_center(Window w) {
    XGetWindowAttributes(d, w, &attr);

    XMoveWindow(d, w, sw / 2 - attr.width  / 2,
                      sh / 2 - attr.height / 2);
}

void win_fs(Window w) {
    client *c;

    for WIN if (c->w == w) {
        if ((c->f = c->f == 0 ? 1 : 0)) {
            XGetWindowAttributes(d, w, &c->a);
            XMoveResizeWindow(d, w, 0, 0, sw, sh);

        } else
            XMoveResizeWindow(d, w, c->a.x, c->a.y, c->a.width, c->a.height);
    }
}

void win_to_ws(const Arg arg) {
    int tmp = ws;
    win_current();

    if (arg.i == tmp) return;

    ws_sel(arg.i);
    win_add(cur);
    ws_save(arg.i);

    ws_sel(tmp);
    win_del(cur);
    XUnmapWindow(d, cur);
    ws_save(tmp);

    if (list) FOC(list->w);
}

void win_next() {
    win_current();
    client *c;

    if (list) {
        for WIN if (c->w == cur) break;

        c = c->next ? c->next : list;

        FOC(c->w);
        XRaiseWindow(d, c->w);
    }
}

void win_fs_current() {
   win_fs(win_current());
}

void win_center_current() {
   win_center(win_current());
}

void ws_go(const Arg arg) {
    client *c;
    int tmp = ws;

    if (arg.i == ws) return;

    ws_save(ws);
    ws_sel(arg.i);

    if (list) for WIN XMapWindow(d, c->w);

    ws_sel(tmp);

    if (list) for WIN XUnmapWindow(d, c->w);

    ws_sel(arg.i);

    if (list) FOC(list->w);
}

void ws_save(int i) {
    ws_list[i].list = list;
}

void ws_sel(int i) {
    list = ws_list[i].list;
    ws = i;
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    wc.x          = ev->x;
    wc.y          = ev->y;
    wc.width      = ev->width;
    wc.height     = ev->height;
    wc.sibling    = ev->above;
    wc.stack_mode = ev->detail;

    XConfigureWindow(d, ev->window, ev->value_mask, &wc);
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;

    XSelectInput(d, w, PropertyChangeMask|StructureNotifyMask|
                       EnterWindowMask|FocusChangeMask);
    win_center(w);
    XMapWindow(d, w);
    FOC(w);
    win_add(w);
}

void run(const Arg arg) {
    if (fork()) return;
    if (d) close(ConnectionNumber(d));

    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

int xerror(Display *d, XErrorEvent *e) {
    return 0;
}

int main(void) {
    XEvent ev;

    if (!(d = XOpenDisplay(0x0))) return 0;

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);

    s    = DefaultScreen(d);
    root = RootWindow(d, s);
    sw   = XDisplayWidth(d, s);
    sh   = XDisplayHeight(d, s);

    key_grab();

    ws_go((Arg){.i = 1});

    XSelectInput(d, root, SubstructureNotifyMask|
        SubstructureRedirectMask|EnterWindowMask|LeaveWindowMask);

    XGrabButton(d, 1, Mod4Mask, root, True,
        ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None);

    XGrabButton(d, 3, Mod4Mask, root, True,
        ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None);

    XDefineCursor(d, root, XCreateFontCursor(d, 68));

    while(1 && !XNextEvent(d, &ev))
        if (events[ev.type]) events[ev.type](&ev);
}
