/*
 *   /\___/\
 *  ( o   o )  Made by cat...
 *  (  =^=  )
 *  (        )            ... for cat!
 *  (         )
 *  (          ))))))________________
 *  ______________________________________________________________________________
 *
 *  Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define TABLENGTH(X)    (sizeof(X)/sizeof(*X))

// Functions
static void die(const char* e);
static unsigned long getcolor(const char* color);
static void grabkeys();
static void keypress(XEvent *e);
static void maprequest(XEvent *e);
static void quit();
static void setup();
static void spawn(const char **command);
static void start();

// Structs
struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const char **command);
    const char **command;
};

struct win_list {
    Window w;
    struct win_list *next;
};

// Include configuration file (need struct key)
#include "config.h"

// Variable
static Display *dis;
static int bool_quit;
static int sh;
static int sw;
static int screen;
static unsigned int win_focus;
static unsigned int win_unfocus;
static Window root;
static struct win_list *win = NULL;

// Events array
static void (*events[LASTEvent])(XEvent *e) = {
    [KeyPress] = keypress,
    [MapRequest] = maprequest
};

void die(const char* e) {
    fprintf(stdout,"%s\n",e);
    exit(1);
}

unsigned long getcolor(const char* color) {
    XColor c;
    Colormap map = DefaultColormap(dis,screen);

    if(!XParseColor(dis,map,color,&c))
        die("catwm: Error parsing color!");

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

void maprequest(XEvent *e) {
    Window w;

    XMapRequestEvent *ev = &e->xmaprequest;
    w = ev->window;

}

void quit() {
    // TODO, close windows, unmap windows,...
    bool_quit = 1;
}

void setup() {
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

    bool_quit = 0;
    
    // To catch maprequest (if other wm running)
    XSelectInput(dis,root,SubstructureRedirectMask);
}

static void spawn(const char **command) {
    if(fork() == 0) {
        execvp((char*)command[0],(char**)command);
        exit(0);
    }
}

void start() {
    XEvent ev;

    // Main loop, just dispatch events (thx to dwm from ;)
    while(!bool_quit && !XNextEvent(dis,&ev)) {
        if(events[ev.type]) {
            events[ev.type](&ev);
        }
    }
}

int main(int argc, char **argv) {
    // Open display   
    if(!(dis = XOpenDisplay(NULL)))
        die("catwm: Cannot open display!");

    // Setup env
    setup();

    // Start wm
    start();

    // Close display
    XCloseDisplay(dis);

    return 0;
}

