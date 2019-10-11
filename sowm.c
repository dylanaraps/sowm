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
    int mode;
};

static void notify_motion(XEvent *e);
static void notify_destroy(XEvent *e);
static void notify_enter(XEvent *e);

static void key_grab();
static void key_press(XEvent *e);

static void button_press(XEvent *e);
static void button_release(XEvent *e);

static void win_add(Window w);
static void win_del(Window w);
static void win_fs();
static void win_kill();
static void win_center();
static void win_next();
static void win_to_ws(const Arg arg);
static void win_update();

static void ws_go(const Arg arg);
static void ws_save(int i);
static void ws_sel(int i);

static void configure_request(XEvent *e);
static void map_request(XEvent *e);
static void run(const Arg arg);

static void wm_init();
static void wm_setup();

static client  *cur, *head;
static desktop desktops[10];

static int curr_desk, mode, screen, sh, sw;

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
    cur     = c;

	XSelectInput(dis, w, PropertyChangeMask|StructureNotifyMask|
                         EnterWindowMask|FocusChangeMask);
}

void ws_go(const Arg arg) {
    client *c;

    if (arg.i == curr_desk)
        return;

    if (head != NULL)
        for(c=head;c;c=c->next) XUnmapWindow(dis,c->win);

    ws_save(curr_desk);
    ws_sel(arg.i);

    if (head != NULL)
        for(c=head;c;c=c->next) XMapWindow(dis,c->win);

    win_update();
}

void win_center() {
    XGetWindowAttributes(dis, cur->win, &attr);

    int x = (sw / 2) - (attr.width  / 2);
    int y = (sh / 2) - (attr.height / 2);

    XMoveWindow(dis, cur->win, x, y);
}

void win_fs() {
    if (cur == NULL) return;

    if (cur->f != 1) {
        XGetWindowAttributes(dis, cur->win, &attr);

        cur->f = 1;
        cur->x = attr.x;
        cur->y = attr.y;
        cur->w = attr.width;
        cur->h = attr.height;

        XMoveResizeWindow(dis, cur->win, 0, 0, sw, sh);
    }

    else {
        cur->f = 0;

        XMoveResizeWindow(dis, cur->win, cur->x, cur->y, cur->w, cur->h);
    }
}

void win_to_ws(const Arg arg) {
    client *tmp = cur;
    int    tmp2 = curr_desk;

    if (arg.i == tmp2 || cur == NULL)
        return;

    // Add client to desktop
    ws_sel(arg.i);
    win_add(tmp->win);
    ws_save(arg.i);

    // Remove client from current desktop
    ws_sel(tmp2);
    XUnmapWindow(dis,tmp->win);
    win_del(tmp->win);
    ws_save(tmp2);
    win_update();
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
    win_update();
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    wc.x            = ev->x;
    wc.y            = ev->y;
    wc.width        = ev->width;
    wc.height       = ev->height;
    wc.sibling      = ev->above;
    wc.stack_mode   = ev->detail;

    XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
}

void win_update() {
    client *c;

    for(c=head;c;c=c->next)
        if (cur == c) {
            XSetInputFocus(dis, c->win, RevertToParent, CurrentTime);
            XRaiseWindow(dis, c->win);
        }
}

void notify_enter(XEvent *e) {
	XCrossingEvent *ev = &e->xcrossing;
    XSetInputFocus(dis, ev->window, RevertToParent, CurrentTime);
}

void key_grab() {
    int i;
    KeyCode code;

    for(i=0;i<TABLENGTH(keys);++i)
        if ((code = XKeysymToKeycode(dis, keys[i].keysym)))
            XGrabKey(dis, code, keys[i].mod, root,
                     True, GrabModeAsync, GrabModeAsync);
}

void key_press(XEvent *e) {
    int i;
    XKeyEvent  ke = e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(dis,ke.keycode,0,0);

    for(i=0;i<TABLENGTH(keys);++i) {
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

        cur->f = 0;
    }
}

void button_release(XEvent *e) {
    start.subwindow = None;
}

void win_kill() {
	if (cur != NULL) XKillClient(dis, cur->win);
}

void map_request(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;
    client *c;

    // For fullscreen mplayer (and maybe some other program)
    for (c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            return;
        }

    win_add(ev->window);
    win_center();
    XMapWindow(dis,ev->window);
    win_update();
}

void win_next() {
    client *c;

    if (cur != NULL && head != NULL) {
		if (cur->next == NULL)
            c = head;
        else
            c = cur->next;

        cur = c;
        win_update();
    }
}

void win_del(Window w) {
    client *c;

    for(c=head;c;c=c->next) {
        if (c->win != w) continue;

        if (c->prev == NULL && c->next == NULL) {
            free(head);

            head = NULL;
            cur  = NULL;

            ws_save(curr_desk);
            return;
        }

        if (c->prev == NULL) {
            head          = c->next;
            c->next->prev = NULL;
            cur           = c->next;
        }

        else if (c->next == NULL) {
            c->prev->next = NULL;
            cur           = c->prev;
        }

        else {
            c->prev->next = c->next;
            c->next->prev = c->prev;
            cur           = c->prev;
        }

        free(c);
        ws_save(curr_desk);
        win_update();
        return;
    }
}

void ws_save(int i) {
    desktops[i].mode    = mode;
    desktops[i].head    = head;
    desktops[i].current = cur;
}

void ws_sel(int i) {
    head = desktops[i].head;
    cur  = desktops[i].current;
    mode = desktops[i].mode;

    curr_desk = i;
}

void wm_setup() {
    int i;

    signal(SIGCHLD, SIG_IGN);

    screen = DefaultScreen(dis);
    root   = RootWindow(dis,screen);
    sw     = XDisplayWidth(dis,screen);
    sh     = XDisplayHeight(dis,screen);

    key_grab();

    mode = 0;
    head = NULL;
    cur  = NULL;

    for(i=0;i<TABLENGTH(desktops);++i) {
        desktops[i].mode    = mode;
        desktops[i].head    = head;
        desktops[i].current = cur;
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
