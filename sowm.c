// sowm
// An itsy bitsy floating window manager
// with roots in catwm.

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define TABLENGTH(X) (sizeof(X)/sizeof(*X))

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

typedef struct desktop desktop;
struct desktop{
    client *head, *current;
};

static void notify_motion(XEvent *e);
static void notify_destroy(XEvent *e);
static void notify_enter(XEvent *e);

static void key_grab();
static void key_press(XEvent *e);

static void button_press(XEvent *e);
static void button_release();

static void win_add(Window w);
static void win_del(Window w);
static void win_fs(Window w);
static void win_fs_current();
static void win_kill();
static void win_center(Window w);
static void win_center_current();
static void win_next();
static void win_to_ws(const Arg arg);
static void win_update(Window w);

static void ws_go(const Arg arg);
static void ws_save(int i);
static void ws_sel(int i);

static void configure_request(XEvent *e);
static void map_request(XEvent *e);
static void run(const Arg arg);

static void wm_init();
static void wm_setup();

static client  *head;
static desktop desktops[10];

static int curr_desk, screen, sh, sw;

static Display *dis;
static Window  root;

static XButtonEvent      start;
static XWindowAttributes attr;

#include "config.h"

static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
    [ConfigureRequest] = configure_request,
    [DestroyNotify]    = notify_destroy,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [MotionNotify]     = notify_motion,
    [EnterNotify]      = notify_enter
};

void win_add(Window w) {
    client *c, *t;

    if (!(c = (client *)calloc(1,sizeof(client))))
        exit(1);

    if (head == NULL) {
        c->prev = NULL;
        head    = c;
    }

    else {
        for (t=head;t->next;t=t->next);

        c->prev = t;
        t->next = c;
    }

    c->next = NULL;
    c->win  = w;
}

void ws_go(const Arg arg) {
    client *c;

    if (arg.i == curr_desk)
        return;

    if (head != NULL)
        for(c=head;c;c=c->next) XUnmapWindow(dis, c->win);

    ws_save(curr_desk);
    ws_sel(arg.i);

    if (head != NULL)
        for(c=head;c;c=c->next) XMapWindow(dis, c->win);
}

Window win_current() {
    Window focused;
    int revert_to;

    XGetInputFocus(dis, &focused, &revert_to);
    return focused;
}

void win_center(Window w) {
    XGetWindowAttributes(dis, w, &attr);

    int x = (sw / 2) - (attr.width  / 2);
    int y = (sh / 2) - (attr.height / 2);

    XMoveWindow(dis, w, x, y);
}

void win_center_current() {
   win_center(win_current());
}

void win_fs_current() {
   win_fs(win_current());
}

void win_fs(Window w) {
    client *c;

    for(c=head;c;c=c->next)
        if (c->win == w) break;

    if (c == NULL) return;

    if (c->f != 1) {
        XGetWindowAttributes(dis, w, &attr);

        c->f = 1;
        c->x = attr.x;
        c->y = attr.y;
        c->w = attr.width;
        c->h = attr.height;

        XMoveResizeWindow(dis, w, 0, 0, sw, sh);
    }

    else {
        c->f = 0;

        XMoveResizeWindow(dis, c->win, c->x, c->y, c->w, c->h);
    }
}

void win_to_ws(const Arg arg) {
    int       tmp  = curr_desk;
    Window current = win_current();

    if (arg.i == tmp)
        return;

    ws_sel(arg.i);
    win_add(current);
    ws_save(arg.i);

    ws_sel(tmp);
    XUnmapWindow(dis, current);
    win_del(current);
}

void notify_destroy(XEvent *e) {
    int i = 0;
    client *c;

    XDestroyWindowEvent *ev = &e->xdestroywindow;

    for (c=head;c;c=c->next)
        if(ev->window == c->win) i++;

    if (i == 0)
        return;

    win_del(ev->window);
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

void win_update(Window w) {
    XSetInputFocus(dis, w, RevertToParent, CurrentTime);
}

void notify_enter(XEvent *e) {
    XSetInputFocus(dis, e->xcrossing.window, RevertToParent, CurrentTime);
}

void key_grab() {
    KeyCode code;

    for(int i=0; i < TABLENGTH(keys); ++i)
        if ((code = XKeysymToKeycode(dis, keys[i].keysym)))
            XGrabKey(dis, code, keys[i].mod, root,
                     True, GrabModeAsync, GrabModeAsync);
}

void key_press(XEvent *e) {
    XKeyEvent  ke = e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(dis,ke.keycode,0,0);

    for(int i=0; i < TABLENGTH(keys); ++i) {
        if (keys[i].keysym == keysym && keys[i].mod == ke.state)
            keys[i].function(keys[i].arg);
    }
}

void button_press(XEvent *e) {
    XButtonEvent bu = e->xbutton;

    if (bu.subwindow != None) {
        XGetWindowAttributes(dis, bu.subwindow, &attr);
        start = bu;
    }
}

void notify_motion(XEvent *e) {
    XButtonEvent bu = e->xbutton;

    if (start.subwindow != None) {
        int xdiff = bu.x_root - start.x_root;
        int ydiff = bu.y_root - start.y_root;

        XMoveResizeWindow(dis, start.subwindow,
            attr.x + (start.button==1 ? xdiff : 0),
            attr.y + (start.button==1 ? ydiff : 0),
            MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
            MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
    }
}

void button_release() {
    start.subwindow = None;
}

void win_kill() {
    Window current = win_current();

    if (current != root)
        XKillClient(dis, current);
}

void map_request(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;

    XSelectInput(dis, ev->window, PropertyChangeMask|StructureNotifyMask|
                                  EnterWindowMask|FocusChangeMask);
    win_add(ev->window);
    win_center(ev->window);
    XMapWindow(dis, ev->window);
    win_update(ev->window);
}

void win_next() {
    Window current = win_current();
    client *c;

    if (head != NULL) {
        for(c=head;c;c=c->next)
            if (c->win == current) break;

        c = c->next;

        if (c == NULL) c = head;

        win_update(c->win);
        XRaiseWindow(dis, c->win);
    }
}

void win_del(Window w) {
    client *c;

    for(c=head;c;c=c->next) {
        if (c->win != w) continue;

        if (c->prev == NULL && c->next == NULL) {
            free(head);

            head = NULL;

            ws_save(curr_desk);
            return;
        }

        if (c->prev == NULL) {
            head          = c->next;
            c->next->prev = NULL;
        }

        else if (c->next == NULL) {
            c->prev->next = NULL;
        }

        else {
            c->prev->next = c->next;
            c->next->prev = c->prev;
        }

        free(c);
        ws_save(curr_desk);
        return;
    }
}

void ws_save(int i) {
    desktops[i].head = head;
}

void ws_sel(int i) {
    head = desktops[i].head;

    curr_desk = i;
}

void wm_setup() {
    signal(SIGCHLD, SIG_IGN);

    screen = DefaultScreen(dis);
    root   = RootWindow(dis, screen);
    sw     = XDisplayWidth(dis, screen);
    sh     = XDisplayHeight(dis, screen);

    key_grab();

    for(int i=0; i < TABLENGTH(desktops); ++i) {
        desktops[i].head    = NULL;
        desktops[i].current = NULL;
    }

    const Arg arg = {.i = 1};
    curr_desk     = arg.i;
    ws_go(arg);

    XSelectInput(dis, root, SubstructureNotifyMask|SubstructureRedirectMask|
                            EnterWindowMask|LeaveWindowMask);
}

void run(const Arg arg) {
    if (fork()) return;
    if (dis)    close(ConnectionNumber(dis));

    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

void wm_init() {
    XEvent ev;

    XGrabKey(dis, XKeysymToKeycode(dis, XStringToKeysym("F1")), Mod4Mask,
            DefaultRootWindow(dis), True, GrabModeAsync, GrabModeAsync);

    XGrabButton(dis, 1, Mod4Mask, DefaultRootWindow(dis), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    XGrabButton(dis, 3, Mod4Mask, DefaultRootWindow(dis), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    start.subwindow = None;

    while(1 && !XNextEvent(dis,&ev))
        if (events[ev.type]) events[ev.type](&ev);
}

int main() {
    if ((dis = XOpenDisplay(NULL))) {
        wm_setup();
        wm_init();

        XCloseDisplay(dis);
    }

    return 0;
}
