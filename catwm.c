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
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

static void win_add(Window w);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void win_mid();
static void win_fs();
static void change_desktop(const Arg arg);
static void win_to_ws(const Arg arg);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void grabkeys();
static void init();
static void keypress(XEvent *e);
static void win_kill();
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void win_next();
static void win_del(Window w);
static void save_desktop(int i);
static void select_desktop(int i);
static void send_kill_signal(Window w);
static void setup();
static void sigchld(int unused);
static void sh(const Arg arg);
static void update_current();

#include "config.h"

static int bool_quit;
static int curr_desk;
static int mode;
static int screen;
static int sh;
static int sw;
static Display *dis;
static Window root;
static XButtonEvent start;
static XWindowAttributes attr;
static client *current;
static client *head;
static desktop desktops[10];

static void (*events[LASTEvent])(XEvent *e) = {
    [KeyPress]         = keypress,
    [MapRequest]       = maprequest,
    [DestroyNotify]    = destroynotify,
    [ConfigureRequest] = configurerequest,
    [ButtonPress]      = buttonpress,
    [ButtonRelease]    = buttonrelease,
    [MotionNotify]     = motionnotify
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

void change_desktop(const Arg arg) {
    client *c;

    if (arg.i == curr_desk)
        return;

    if (head != NULL)
        for(c=head;c;c=c->next) XUnmapWindow(dis,c->win);

    save_desktop(curr_desk);
    select_desktop(arg.i);

    if (head != NULL)
        for(c=head;c;c=c->next) XMapWindow(dis,c->win);

    update_current();
}

void win_mid() {
    XGetWindowAttributes(dis, current->win, &attr);

    int x = (sw / 2) - (attr.width  / 2);
    int y = (sh / 2) - (attr.height / 2);

    XMoveWindow(dis, current->win, x, y);
}

void win_fs() {
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
    select_desktop(arg.i);
    win_add(tmp->win);
    save_desktop(arg.i);

    // Remove client from current desktop
    select_desktop(tmp2);
    XUnmapWindow(dis,tmp->win);
    win_del(tmp->win);
    save_desktop(tmp2);
    update_current();
}

void configurerequest(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    wc.x            = ev->x;
    wc.y            = ev->y;
    wc.width        = ev->width;
    wc.height       = ev->height;
    wc.border_width = ev->border_width;
    wc.sibling      = ev->above;
    wc.stack_mode   = ev->detail;

    XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
}

void destroynotify(XEvent *e) {
    int i = 0;
    client *c;

    XDestroyWindowEvent *ev = &e->xdestroywindow;

    for(c=head;c;c=c->next)
        if(ev->window == c->win) i++;

    if(i == 0)
        return;

    win_del(ev->window);
    update_current();
}

void update_current() {
    client *c;

    for(c=head;c;c=c->next)
        if (current == c) {
            XSetInputFocus(dis, c->win, RevertToParent, CurrentTime);
            XRaiseWindow(dis, c->win);
        }
}

void grabkeys() {
    int i;
    KeyCode code;

    for(i=0;i<TABLENGTH(keys);++i) {
        if ((code = XKeysymToKeycode(dis, keys[i].keysym)))
            XGrabKey(dis, code, keys[i].mod, root,
                     True, GrabModeAsync, GrabModeAsync);
    }
}

void keypress(XEvent *e) {
    int i;
    XKeyEvent ke = e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(dis,ke.keycode,0,0);

    for(i=0;i<TABLENGTH(keys);++i) {
        if(keys[i].keysym == keysym && keys[i].mod == ke.state) {
            keys[i].function(keys[i].arg);
        }
    }
}

void buttonpress(XEvent *e) {
    XButtonEvent bu = e->xbutton;

    if (bu.subwindow != None) {
        XGetWindowAttributes(dis, bu.subwindow, &attr);
        start = bu;
    }
}

void motionnotify(XEvent *e) {
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

void buttonrelease(XEvent *e) {
    start.subwindow = None;
}

void win_kill() {
	if(current != NULL) {
		//send delete signal to window
		XEvent ke;

		ke.type                 = ClientMessage;
		ke.xclient.window       = current->win;
		ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
		ke.xclient.format       = 32;
		ke.xclient.data.l[0]    = XInternAtom(dis, "WM_DELETE_WINDOW", True);
		ke.xclient.data.l[1]    = CurrentTime;

		XSendEvent(dis, current->win, False, NoEventMask, &ke);
		send_kill_signal(current->win);
	}
}

void maprequest(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;
    client *c;

    // For fullscreen mplayer (and maybe some other program)
    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            return;
        }

    win_add(ev->window);
    XMapWindow(dis,ev->window);
    update_current();
}

void win_next() {
    client *c;

    if (current != NULL && head != NULL) {
		if (current->next == NULL)
            c = head;
        else
            c = current->next;

        current = c;
        update_current();
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

                save_desktop(curr_desk);
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
            save_desktop(curr_desk);
            update_current();
            return;
        }
    }
}

void save_desktop(int i) {
    desktops[i].mode    = mode;
    desktops[i].head    = head;
    desktops[i].current = current;
}

void select_desktop(int i) {
    head      = desktops[i].head;
    current   = desktops[i].current;
    mode      = desktops[i].mode;
    curr_desk = i;
}

void send_kill_signal(Window w) {
    XEvent ke;

    ke.type                 = ClientMessage;
    ke.xclient.window       = w;
    ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
    ke.xclient.format       = 32;
    ke.xclient.data.l[0]    = XInternAtom(dis, "WM_DELETE_WINDOW", True);
    ke.xclient.data.l[1]    = CurrentTime;

    XSendEvent(dis, w, False, NoEventMask, &ke);
}

void setup() {
    int i;

    sigchld(0);

    screen = DefaultScreen(dis);
    root   = RootWindow(dis,screen);
    sw     = XDisplayWidth(dis,screen);
    sh     = XDisplayHeight(dis,screen);

    grabkeys();

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
    change_desktop(arg);

    XSelectInput(dis, root, SubstructureNotifyMask|SubstructureRedirectMask);
}

void sigchld(int unused) {
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
        exit(1);

	while(0 < waitpid(-1, NULL, WNOHANG));
}

void sh(const Arg arg) {
    if (fork() == 0) {
        if (fork() == 0) {
            if (dis) close(ConnectionNumber(dis));

            setsid();
            execvp((char*)arg.com[0],(char**)arg.com);
        }

        exit(0);
    }
}

void init() {
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

    setup();
    init();
    XCloseDisplay(dis);

    return 0;
}

