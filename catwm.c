 /*
 *   /\___/\
 *  ( o   o )  Made by cat...
 *  (  =^=  )
 *  (        )            ... for cat!
 *  (         )
 *  (          ))))))________________ Cute And Tiny Window Manager
 *  ______________________________________________________________________________
 *
 *  Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
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

typedef union {
    const char** com;
    const int i;
} Arg;

// Structs
struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const Arg arg);
    const Arg arg;
};

typedef struct client client;
struct client{
    // Prev and next client
    client *next;
    client *prev;

    // The window
    Window win;
};

typedef struct desktop desktop;
struct desktop{
    int master_size;
    int mode;
    client *head;
    client *current;
};

// Functions
static void add_window(Window w);
static void change_desktop(const Arg arg);
static void client_to_desktop(const Arg arg);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void die(const char* e);
static void grabkeys();
static void keypress(XEvent *e);
static void kill_client();
static void maprequest(XEvent *e);
static void next_win();
static void remove_window(Window w);
static void save_desktop(int i);
static void select_desktop(int i);
static void send_kill_signal(Window w);
static void setup();
static void sigchld(int unused);
static void spawn(const Arg arg);
static void start();

// Include configuration file (need struct key)
#include "config.h"

// Variable
static Display *dis;
static int bool_quit;
static int current_desktop;
static int master_size;
static int mode;
static int sh;
static int sw;
static int screen;
static Window root;
static client *head;
static client *current;

// Events array
static void (*events[LASTEvent])(XEvent *e) = {
    [KeyPress] = keypress,
    [MapRequest] = maprequest,
    [DestroyNotify] = destroynotify,
    [ConfigureNotify] = configurenotify,
    [ConfigureRequest] = configurerequest
};

// Desktop array
static desktop desktops[10];

void add_window(Window w) {
    client *c,*t;

    if(!(c = (client *)calloc(1,sizeof(client))))
        die("Error calloc!");

    if(head == NULL) {
        c->next = NULL;
        c->prev = NULL;
        c->win = w;
        head = c;
    }
    else {
        for(t=head;t->next;t=t->next);

        c->next = NULL;
        c->prev = t;
        c->win = w;

        t->next = c;
    }

    current = c;
}

void change_desktop(const Arg arg) {
    client *c;

    if(arg.i == current_desktop)
        return;

    // Unmap all window
    if(head != NULL)
        for(c=head;c;c=c->next)
            XUnmapWindow(dis,c->win);

    // Save current "properties"
    save_desktop(current_desktop);

    // Take "properties" from the new desktop
    select_desktop(arg.i);

    // Map all windows
    if(head != NULL)
        for(c=head;c;c=c->next)
            XMapWindow(dis,c->win);
}

void client_to_desktop(const Arg arg) {
    client *tmp = current;
    int tmp2 = current_desktop;

    if(arg.i == current_desktop || current == NULL)
        return;

    // Add client to desktop
    select_desktop(arg.i);
    add_window(tmp->win);
    save_desktop(arg.i);

    // Remove client from current desktop
    select_desktop(tmp2);
    remove_window(current->win);
}

void configurenotify(XEvent *e) {
    // Do nothing for the moment
}

void configurerequest(XEvent *e) {
    // Paste from DWM, thx again \o/
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;
    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dis, ev->window, ev->value_mask, &wc);
}

void destroynotify(XEvent *e) {
    int i=0;
    client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    // Uber (and ugly) hack ;)
    for(c=head;c;c=c->next)
        if(ev->window == c->win)
            i++;

    // End of the hack
    if(i == 0)
        return;

    remove_window(ev->window);
}

void die(const char* e) {
    fprintf(stdout,"catwm: %s\n",e);
    exit(1);
}

void grabkeys() {
    int i;
    KeyCode code;

    // For each shortcuts
    for(i=0;i<TABLENGTH(keys);++i) {
        if((code = XKeysymToKeycode(dis,keys[i].keysym))) {
            XGrabKey(dis,code,keys[i].mod,root,True,GrabModeAsync,GrabModeAsync);
        }
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

void kill_client() {
	if(current != NULL) {
		//send delete signal to window
		XEvent ke;
		ke.type = ClientMessage;
		ke.xclient.window = current->win;
		ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
		ke.xclient.format = 32;
		ke.xclient.data.l[0] = XInternAtom(dis, "WM_DELETE_WINDOW", True);
		ke.xclient.data.l[1] = CurrentTime;
		XSendEvent(dis, current->win, False, NoEventMask, &ke);
		send_kill_signal(current->win);
	}
}

void maprequest(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;

    // For fullscreen mplayer (and maybe some other program)
    client *c;
    for(c=head;c;c=c->next)
        if(ev->window == c->win) {
            XMapWindow(dis,ev->window);
            return;
        }

    add_window(ev->window);
    XMapWindow(dis,ev->window);
}

void next_win() {
    client *c;

    if(current != NULL && head != NULL) {
		if(current->next == NULL)
            c = head;
        else
            c = current->next;

        current = c;
    }
}

void remove_window(Window w) {
    client *c;

    // CHANGE THIS UGLY CODE
    for(c=head;c;c=c->next) {

        if(c->win == w) {
            if(c->prev == NULL && c->next == NULL) {
                free(head);
                head = NULL;
                current = NULL;
                return;
            }

            if(c->prev == NULL) {
                head = c->next;
                c->next->prev = NULL;
                current = c->next;
            }
            else if(c->next == NULL) {
                c->prev->next = NULL;
                current = c->prev;
            }
            else {
                c->prev->next = c->next;
                c->next->prev = c->prev;
                current = c->prev;
            }

            free(c);
            return;
        }
    }
}

void save_desktop(int i) {
    desktops[i].master_size = master_size;
    desktops[i].mode = mode;
    desktops[i].head = head;
    desktops[i].current = current;
}

void select_desktop(int i) {
    head = desktops[i].head;
    current = desktops[i].current;
    master_size = desktops[i].master_size;
    mode = desktops[i].mode;
    current_desktop = i;
}

void send_kill_signal(Window w) {
    XEvent ke;
    ke.type = ClientMessage;
    ke.xclient.window = w;
    ke.xclient.message_type = XInternAtom(dis, "WM_PROTOCOLS", True);
    ke.xclient.format = 32;
    ke.xclient.data.l[0] = XInternAtom(dis, "WM_DELETE_WINDOW", True);
    ke.xclient.data.l[1] = CurrentTime;
    XSendEvent(dis, w, False, NoEventMask, &ke);
}

void setup() {
    // Install a signal
    sigchld(0);

    // Screen and root window
    screen = DefaultScreen(dis);
    root = RootWindow(dis,screen);

    // Screen width and height
    sw = XDisplayWidth(dis,screen);
    sh = XDisplayHeight(dis,screen);

    // Shortcuts
    grabkeys();

    // Vertical stack
    mode = 0;

    // For exiting
    bool_quit = 0;

    // List of client
    head = NULL;
    current = NULL;

    // Master size
    master_size = sw*MASTER_SIZE;

    // Set up all desktop
    int i;
    for(i=0;i<TABLENGTH(desktops);++i) {
        desktops[i].master_size = master_size;
        desktops[i].mode = mode;
        desktops[i].head = head;
        desktops[i].current = current;
    }

    // Select first dekstop by default
    const Arg arg = {.i = 1};
    current_desktop = arg.i;
    change_desktop(arg);

    // To catch maprequest and destroynotify (if other wm running)
    XSelectInput(dis,root,SubstructureNotifyMask|SubstructureRedirectMask);
}

void sigchld(int unused) {
    // Again, thx to dwm ;)
	if(signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void spawn(const Arg arg) {
    if(fork() == 0) {
        if(fork() == 0) {
            if(dis)
                close(ConnectionNumber(dis));

            setsid();
            execvp((char*)arg.com[0],(char**)arg.com);
        }
        exit(0);
    }
}

void start() {
    XEvent ev;

    // Main loop, just dispatch events (thx to dwm ;)
    while(!bool_quit && !XNextEvent(dis,&ev)) {
        if(events[ev.type])
            events[ev.type](&ev);
    }
}

int main(int argc, char **argv) {
    if(!(dis = XOpenDisplay(NULL)))
        die("Cannot open display!");

    setup();
    start();
    XCloseDisplay(dis);

    return 0;
}

