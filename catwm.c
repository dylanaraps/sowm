 /*
 *   /\___/\
 *  ( o   o )  Made by cat...
 *  (  =^=  )
 *  (        )            ... for cat!
 *  (         )
 *  (          ))))))________________
 *  __________________
 */

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
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
    client *next;
    client *prev;
    Window win;

    int f, x, y, w, h;
};

typedef struct desktop desktop;
struct desktop{
    int mode;
    client *head;
    client *current;
};

static void configure_request(XEvent *e);
static void destroy_notify(XEvent *e);
static void map_request(XEvent *e);

static void run(const Arg arg);

static void sig_child(int unused);

static void key_grab();
static void key_press(XEvent *e);

static void button_press(XEvent *e);
static void button_release(XEvent *e);
static void motion_notify(XEvent *e);

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

static void wm_init();
static void wm_setup();

static client  *current;
static client  *head;
static desktop desktops[10];

static int bool_quit;
static int curr_desk;
static int mode;
static int screen;
static int sh;
static int sw;

static Display *dis;
static Window  root;

static XButtonEvent      start;
static XWindowAttributes attr;

#include "config.h"

static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
    [ConfigureRequest] = configure_request,
    [DestroyNotify]    = destroy_notify,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [MotionNotify]     = motion_notify
};

void win_add(Window w) {
    client *c, *t;

    if (!(c = (client *)calloc(1,sizeof(client))))
        exit(1);

    if (head == NULL) {
        c->next = NULL;
        c->prev = NULL;
        c->win  = w;
        head    = c;
    }

    else {
        for(t=head;t->next;t=t->next);

        c->next = NULL;
        c->prev = t;
        c->win  = w;
        t->next = c;
    }

    current = c;
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
    XGetWindowAttributes(dis, current->win, &attr);

    int x = (sw / 2) - (attr.width  / 2);
    int y = (sh / 2) - (attr.height / 2);

    XMoveWindow(dis, current->win, x, y);
}

void win_fs() {
    if (current == NULL) return;

    if (current->f != 1) {
        XGetWindowAttributes(dis, current->win, &attr);

        current->f = 1;
        current->x = attr.x;
        current->y = attr.y;
        current->w = attr.width;
        current->h = attr.height;

        XMoveResizeWindow(dis, current->win, 0, 0, sw, sh);
    }

    else {
        current->f = 0;

        XMoveResizeWindow(dis, current->win, current->x, current->y, \
                                             current->w, current->h);
    }
}

void win_to_ws(const Arg arg) {
    client *tmp = current;
    int    tmp2 = curr_desk;

    if (arg.i == tmp2 || current == NULL)
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

void destroy_notify(XEvent *e) {
    int i = 0;
    client *c;

    XDestroyWindowEvent *ev = &e->xdestroywindow;

    for(c=head;c;c=c->next)
        if(ev->window == c->win) i++;

    if(i == 0)
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
        if (current == c) {
            XSetInputFocus(dis, c->win, RevertToParent, CurrentTime);
            XRaiseWindow(dis, c->win);
        }
}

void key_grab() {
    int i;
    KeyCode code;

    for(i=0;i<TABLENGTH(keys);++i) {
        if ((code = XKeysymToKeycode(dis, keys[i].keysym)))
            XGrabKey(dis, code, keys[i].mod, root,
                     True, GrabModeAsync, GrabModeAsync);
    }
}

void key_press(XEvent *e) {
    int i;
    XKeyEvent ke = e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(dis,ke.keycode,0,0);

    for(i=0;i<TABLENGTH(keys);++i) {
        if(keys[i].keysym == keysym && keys[i].mod == ke.state) {
            keys[i].function(keys[i].arg);
        }
    }
}

void button_press(XEvent *e) {
    XButtonEvent bu = e->xbutton;

    if (bu.subwindow != None) {
        XGetWindowAttributes(dis, bu.subwindow, &attr);
        start = bu;
    }
}

void motion_notify(XEvent *e) {
    XButtonEvent bu = e->xbutton;

    if (start.subwindow != None) {
        int xdiff = bu.x_root - start.x_root;
        int ydiff = bu.y_root - start.y_root;

        XMoveResizeWindow(dis, start.subwindow,
            attr.x + (start.button==1 ? xdiff : 0),
            attr.y + (start.button==1 ? ydiff : 0),
            MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
            MAX(1, attr.height + (start.button==3 ? ydiff : 0)));

        current->f = 0;
    }
}

void button_release(XEvent *e) {
    start.subwindow = None;
}

void win_kill() {
	if(current != NULL)
        XKillClient(dis, current->win);
}

void map_request(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;
    client *c;

    // For fullscreen mplayer (and maybe some other program)
    for(c=head;c;c=c->next)
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

    if (current != NULL && head != NULL) {
		if (current->next == NULL)
            c = head;
        else
            c = current->next;

        current = c;
        win_update();
    }
}

void win_del(Window w) {
    client *c;

    for(c=head;c;c=c->next) {
        if(c->win == w) {
            if (c->prev == NULL && c->next == NULL) {
                free(head);

                head    = NULL;
                current = NULL;

                ws_save(curr_desk);
                return;
            }

            if (c->prev == NULL) {
                head          = c->next;
                c->next->prev = NULL;
                current       = c->next;
            }

            else if (c->next == NULL) {
                c->prev->next = NULL;
                current       = c->prev;
            }

            else {
                c->prev->next = c->next;
                c->next->prev = c->prev;
                current       = c->prev;
            }

            free(c);
            ws_save(curr_desk);
            win_update();
            return;
        }
    }
}

void ws_save(int i) {
    desktops[i].mode    = mode;
    desktops[i].head    = head;
    desktops[i].current = current;
}

void ws_sel(int i) {
    head      = desktops[i].head;
    current   = desktops[i].current;
    mode      = desktops[i].mode;
    curr_desk = i;
}

void wm_setup() {
    int i;

    sig_child(0);

    screen = DefaultScreen(dis);
    root   = RootWindow(dis,screen);
    sw     = XDisplayWidth(dis,screen);
    sh     = XDisplayHeight(dis,screen);

    key_grab();

    mode      = 0;
    bool_quit = 0;
    head      = NULL;
    current   = NULL;

    for(i=0;i<TABLENGTH(desktops);++i) {
        desktops[i].mode    = mode;
        desktops[i].head    = head;
        desktops[i].current = current;
    }

    const Arg arg = {.i = 1};
    curr_desk = arg.i;
    ws_go(arg);

    XSelectInput(dis, root, SubstructureNotifyMask|SubstructureRedirectMask);
}

void sig_child(int unused) {
	if (signal(SIGCHLD, sig_child) == SIG_ERR)
        exit(1);

	while(0 < waitpid(-1, NULL, WNOHANG));
}

void run(const Arg arg) {
    if (fork() == 0) {
        if (fork() == 0) {
            if (dis) close(ConnectionNumber(dis));

            setsid();
            execvp((char*)arg.com[0],(char**)arg.com);
        }

        exit(0);
    }
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

    while(!bool_quit && !XNextEvent(dis,&ev))
        if (events[ev.type]) events[ev.type](&ev);
}

int main(int argc, char **argv) {
    if (!(dis = XOpenDisplay(NULL)))
        exit(1);

    wm_setup();
    wm_init();

    XCloseDisplay(dis);
    return 0;
}

