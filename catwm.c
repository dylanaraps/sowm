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
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))

// Structs
struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const char **command);
    const char **command;
};

typedef struct client client;
struct client{
    // Prev and next client
    client *next;
    client *prev;

    // The window
    Window win;
};

// Functions
static void add_window(Window w);
static void configurenotify(XEvent *e);
static void decrease();
static void destroynotify(XEvent *e);
static void die(const char* e);
static unsigned long getcolor(const char* color);
static void grabkeys();
static void increase();
static void keypress(XEvent *e);
static void kill_client();
static void maprequest(XEvent *e);
static void next_win();
static void prev_win();
static void quit();
static void remove_window(Window w);
static void setup();
static void sigchld(int unused);
static void spawn(const char **command);
static void start();
static void swap_master();
static void switch_mode();
static void tile();
static void update_current();

// Include configuration file (need struct key)
#include "config.h"

// Variable
static Display *dis;
static int bool_quit;
static int master_size;
static int mode;
static int sh;
static int sw;
static int screen;
static unsigned int win_focus;
static unsigned int win_unfocus;
static Window root;
static client *head;
static client *current;

// Events array
static void (*events[LASTEvent])(XEvent *e) = {
    [KeyPress] = keypress,
    [MapRequest] = maprequest,
    [DestroyNotify] = destroynotify,
    [ConfigureNotify] = configurenotify
};

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

void configurenotify(XEvent *e) {
    // Do nothing for the moment
}

void decrease() {
    if(master_size > 50) {
        master_size -= 10;
        tile();
    }
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
    tile();
    update_current();
}

void die(const char* e) {
    fprintf(stdout,"catwm: %s\n",e);
    exit(1);
}

unsigned long getcolor(const char* color) {
    XColor c;
    Colormap map = DefaultColormap(dis,screen);

    if(!XAllocNamedColor(dis,map,color,&c,&c))
        die("Error parsing color!");

    return c.pixel;
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

void increase() {
    if(master_size < sw-50) {
        master_size += 10;
        tile();
    }
}

void keypress(XEvent *e) {
    int i;
    XKeyEvent ke = e->xkey;
    KeySym keysym = XKeycodeToKeysym(dis,ke.keycode,0);

    for(i=0;i<TABLENGTH(keys);++i) {
        if(keys[i].keysym == keysym && keys[i].mod == ke.state) {
            keys[i].function(keys[i].command);
        }
    }
}

void kill_client() {
    if(current != NULL) 
        XKillClient(dis, current->win);
}

void maprequest(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;
    
    add_window(ev->window);
    XMapWindow(dis,ev->window);
    tile();
    update_current();
}

void next_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(current->next == NULL)
            c = head;
        else
            c = current->next;

        current = c;
        update_current();
    }
}

void prev_win() {
    client *c;

    if(current != NULL && head != NULL) {
        if(current->prev == NULL)
            for(c=head;c->next;c=c->next);
        else
            c = current->prev;

        current = c;
        update_current();
    }
}

void quit() {
    XUngrabKey(dis,AnyKey,AnyModifier,root);
    XDestroySubwindows(dis,root);
    fprintf(stdout,"catwm: Thanks for using!\n");
    bool_quit = 1;
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

void setup() {
    // Install a signal
    sigchld(0);

    // Screen and root window
    screen = DefaultScreen(dis);
    root = RootWindow(dis,screen);

    // Screen width and height
    sw = XDisplayWidth(dis,screen);
    sh = XDisplayHeight(dis,screen);

    // Colors
    win_focus = getcolor(FOCUS);
    win_unfocus = getcolor(UNFOCUS);

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
    
    // To catch maprequest and destroynotify (if other wm running)
    XSelectInput(dis,root,SubstructureNotifyMask|SubstructureRedirectMask);
}

void sigchld(int unused) {
    // Again, thx to dwm ;)
	if(signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void spawn(const char **command) {
    if(fork() == 0) {
        if(fork() == 0) {
            if(dis)
                close(ConnectionNumber(dis));

            setsid();
            execvp((char*)command[0],(char**)command);
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

void swap_master() {
    Window tmp;

    if(head != NULL && current != NULL && current != head && mode == 0) {
        tmp = head->win;
        head->win = current->win;
        current->win = tmp;
        current = head;

        tile();
        update_current();
    }
}

void switch_mode() {
    mode = (mode == 0) ? 1:0;
    tile();
    update_current();
}

void tile() {
    client *c;
    int n = 0;
    int y = 0;

    // If only one window
    if(head != NULL && head->next == NULL) {
        XMoveResizeWindow(dis,head->win,0,0,sw-2,sh-2);
    }
    else if(head != NULL) {
        switch(mode) {
            case 0:
                // Master window
                XMoveResizeWindow(dis,head->win,0,0,master_size-2,sh-2);

                // Stack
                for(c=head->next;c;c=c->next) ++n;
                for(c=head->next;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,master_size,y,sw-master_size-2,(sh/n)-2);
                    y += sh/n;
                }
                break;
            case 1:
                for(c=head;c;c=c->next) {
                    XMoveResizeWindow(dis,c->win,0,0,sw,sh);
                }
                break;
            default:
                break;
        }
    }
}

void update_current() {
    client *c;

    for(c=head;c;c=c->next)
        if(current == c) {
            // "Enable" current window
            XSetWindowBorderWidth(dis,c->win,1);
            XSetWindowBorder(dis,c->win,win_focus);
            XSetInputFocus(dis,c->win,RevertToParent,CurrentTime);
            XRaiseWindow(dis,c->win);
        }
        else
            XSetWindowBorder(dis,c->win,win_unfocus);
}

int main(int argc, char **argv) {
    // Open display   
    if(!(dis = XOpenDisplay(NULL)))
        die("Cannot open display!");

    // Setup env
    setup();

    // Start wm
    start();

    // Close display
    XCloseDisplay(dis);

    return 0;
}

