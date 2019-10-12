// sowm
// An itsy bitsy floating window manager
// with roots in catwm.

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define WIN (c=head;c;c=c->next)

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
    Window win;
    int f, x, y, w, h;
};

typedef struct ws ws;
struct ws{client *head;};

static void notify_destroy(XEvent *e);
static void notify_enter(XEvent *e);
static void notify_motion(XEvent *e);

static void key_grab();
static void key_press(XEvent *e);

static void button_press(XEvent *e);
static void button_release();

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

static void configure_request(XEvent *e);
static void map_request(XEvent *e);
static void run(const Arg arg);

static void wm_init();
static void wm_setup();

static client  *head;
static ws ws_list[10];

static int desk = 1, sh, sw;

static Display *dis;
static Window  root;

static XButtonEvent      start;
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

void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);
}

void notify_enter(XEvent *e) {
    XSetInputFocus(dis, e->xcrossing.window, RevertToParent, CurrentTime);
}

void notify_motion(XEvent *e) {
    if (start.subwindow != None) {
        int xdiff = e->xbutton.x_root - start.x_root;
        int ydiff = e->xbutton.y_root - start.y_root;

        XMoveResizeWindow(dis, start.subwindow,
            attr.x + (start.button==1 ? xdiff : 0),
            attr.y + (start.button==1 ? ydiff : 0),
            attr.width  + (start.button==3 ? xdiff : 0),
            attr.height + (start.button==3 ? ydiff : 0));
    }
}

void key_grab() {
    KeyCode code;

    for(int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        if ((code = XKeysymToKeycode(dis, keys[i].keysym)))
            XGrabKey(dis, code, keys[i].mod, root,
                     True, GrabModeAsync, GrabModeAsync);
}

void key_press(XEvent *e) {
    XKeyEvent  ke = e->xkey;
    KeySym keysym = XKeycodeToKeysym(dis,ke.keycode,0);

    for(int i=0; i < sizeof(keys)/sizeof(*keys); ++i) {
        if (keys[i].keysym == keysym && keys[i].mod == ke.state)
            keys[i].function(keys[i].arg);
    }
}

void button_press(XEvent *e) {
    XButtonEvent bu = e->xbutton;

    if (bu.subwindow != None) {
        XGetWindowAttributes(dis, bu.subwindow, &attr);
        XRaiseWindow(dis, bu.subwindow);
        start = bu;
    }
}

void button_release() {
    start.subwindow = None;
}

Window win_current() {
    Window focused;
    int revert_to;

    XGetInputFocus(dis, &focused, &revert_to);
    return focused;
}

void win_add(Window w) {
    client *c, *t;

    if (!(c = (client *)calloc(1,sizeof(client))))
        exit(1);

    if (!head) {
        c->next = 0;
        c->prev = 0;
        c->win  = w;
        head    = c;

    } else {
        for (t=head;t->next;t=t->next);

        c->next = 0;
        c->prev = t;
        c->win  = w;
        t->next = c;
    }

    ws_save(desk);
}

void win_del(Window w) {
    client *c;

    for WIN {
        if (c->win != w) continue;

        if (!c->prev && !c->next) {
            free(head);
            head = 0;
            ws_save(desk);
            return;
        }

        if (!c->prev) {
            head          = c->next;
            c->next->prev = 0;

        } else if (!c->next) {
            c->prev->next = 0;

        } else {
            c->prev->next = c->next;
            c->next->prev = c->prev;
        }

        free(c);
        ws_save(desk);
        return;
    }
}

void win_kill() {
    Window cur = win_current();

    if (cur != root) XKillClient(dis, cur);
}

void win_center(Window w) {
    XGetWindowAttributes(dis, w, &attr);

    int x = (sw / 2) - (attr.width  / 2);
    int y = (sh / 2) - (attr.height / 2);

    XMoveWindow(dis, w, x, y);
}

void win_fs(Window w) {
    client *c;

    for WIN if (c->win == w) break;

    if (!c) return;

    if (!c->f) {
        XGetWindowAttributes(dis, w, &attr);

        c->f = 1;
        c->x = attr.x;
        c->y = attr.y;
        c->w = attr.width;
        c->h = attr.height;

        XMoveResizeWindow(dis, w, 0, 0, sw, sh);

    } else {
        c->f = 0;

        XMoveResizeWindow(dis, w, c->x, c->y, c->w, c->h);
    }
}

void win_to_ws(const Arg arg) {
    int    tmp = desk;
    Window cur = win_current();

    if (arg.i == tmp) return;

    ws_sel(arg.i);
    win_add(cur);
    ws_save(arg.i);

    ws_sel(tmp);
    XUnmapWindow(dis, cur);
    win_del(cur);
    ws_save(tmp);
}

void win_next() {
    Window cur = win_current();
    client *c;

    if (!head) return;
    if (cur == root) cur = head->win;

    for WIN if (c->win == cur) break;

    if ((c = c->next ? c->next : head)) {
        XSetInputFocus(dis, c->win, RevertToParent, CurrentTime);
        XRaiseWindow(dis, c->win);
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
    int tmp = desk;

    if (arg.i == desk) return;

    ws_save(desk);
    ws_sel(arg.i);

    if (head) for WIN XMapWindow(dis, c->win);

    ws_sel(tmp);

    if (head) for WIN XUnmapWindow(dis, c->win);

    ws_sel(arg.i);
}

void ws_save(int i) {
    ws_list[i].head = head;
}

void ws_sel(int i) {
    head = ws_list[i].head;
    desk = i;
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

    XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
}

void map_request(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;

    XSelectInput(dis, ev->window, PropertyChangeMask|StructureNotifyMask|
                                  EnterWindowMask|FocusChangeMask);
    win_center(ev->window);
    XMapWindow(dis, ev->window);
    XSetInputFocus(dis, ev->window, RevertToParent, CurrentTime);
    win_add(ev->window);
}

void run(const Arg arg) {
    if (fork()) return;
    if (dis)    close(ConnectionNumber(dis));

    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

void wm_setup() {
    signal(SIGCHLD, SIG_IGN);

    int s = DefaultScreen(dis);
    root  = RootWindow(dis, s);
    sw    = XDisplayWidth(dis, s);
    sh    = XDisplayHeight(dis, s);

    key_grab();

    for(int i=0; i < sizeof(ws_list)/sizeof(*ws_list); ++i)
        ws_list[i].head = 0;

    const Arg arg = {.i = 1};
    ws_go(arg);

    XSelectInput(dis, root, SubstructureNotifyMask|
        SubstructureRedirectMask|EnterWindowMask|LeaveWindowMask);
}

void wm_init() {
    XEvent ev;

    wm_setup();

    XGrabButton(dis, 1, Mod4Mask, root, True,
        ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None);

    XGrabButton(dis, 3, Mod4Mask, root, True,
        ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
        GrabModeAsync, GrabModeAsync, None, None);

    start.subwindow = None;

    while(1 && !XNextEvent(dis, &ev))
        if (events[ev.type]) events[ev.type](&ev);
}

int main(void) {
    if ((dis = XOpenDisplay(0))) wm_init();

    return 0;
}
