/*
    Copyright (c) 2010, Rinaldini Julien, julien.rinaldini@heig-vd.ch
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
       * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
          names of its contributors may be used to endorse or promote products
          derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MOD     Mod1Mask
#define MOD2    ShiftMask
#define MASTER_AREA_SIZE 0.6

// Key functions
enum key_func{
    k_quit,
    k_spawn_custom,
    k_close,
    k_change_mode,
    k_next_win,
    k_swap_master
};

// To store a key
struct keys {
    unsigned int mod;
    int keysym;
    enum key_func funct;
    const char* command;
};

// To store keybindings
// Create your own here
struct keys key_bindings[] = {
    {MOD,XK_q,k_quit,NULL},
    {MOD,XK_x,k_close,NULL},
    {MOD,XK_Tab,k_next_win,NULL},
    {MOD,XK_space,k_change_mode,NULL},
    {MOD,XK_p,k_spawn_custom,"dmenu_run"},
    {MOD|MOD2,XK_Return,k_spawn_custom,"xterm"}
};

// Create shorcuts for events
void create_shortcuts(Display *dis,Window root) {
    KeyCode code;
    int i;
    for(i=0;i< sizeof key_bindings / sizeof *key_bindings;++i) {
        code = XKeysymToKeycode(dis, key_bindings[i].keysym);
        XGrabKey(dis,code,key_bindings[i].mod,root, True, GrabModeAsync, GrabModeAsync);
    }
}

// Allow to run a program
void spawn(const char *command) {
    int pid = fork ();

    if (pid > 0) {
        return ;
    } else if (pid == 0) {
        system(command);
    } else {
        perror ("FORK error: ");
    }
}

// To tile windows
void tile_windows(Display *dis,Window root,int stack_mode) {
    Window parent;
    Window *children;
    unsigned int n;

    XQueryTree (dis, root, &root, &parent, &children, &n);
    
    int width = XDisplayWidth(dis,0);
    int height = XDisplayHeight(dis,0);

    if(n == 1) {
        XMoveWindow(dis,children[0],0,0);
        XResizeWindow(dis,children[0],width,height);
    }
    else if(n > 1) {
        switch(stack_mode) {
            case 0:
                {
                    int i;
                    int pos_y = 0;
                    int size = height/(n-1);
 
                    XResizeWindow(dis,children[0],width*MASTER_AREA_SIZE,height);

                    for(i=1;i<n;++i) {
                        XMoveWindow(dis,children[i],width*MASTER_AREA_SIZE,pos_y);
                        XResizeWindow(dis,children[i],width-(width*MASTER_AREA_SIZE),size);
                        XMapWindow(dis,children[i]);
                        pos_y += size;
                    }
                }
                break;
            case 1:
                 {
                    int i;
                    int pos_x = 0;
                    int size = width/(n-1);
    
                    XResizeWindow(dis,children[0],width,height*MASTER_AREA_SIZE);

                    for(i=1;i<n;++i) {
                        XMoveWindow(dis,children[i],pos_x,height*MASTER_AREA_SIZE);
                        XResizeWindow(dis,children[i],size,height-(height*MASTER_AREA_SIZE));
                        XMapWindow(dis,children[i]);
                        pos_x += size;
                    }
                }
                break;
            case 2:
                {
                    int i;
                    for(i=0;i<n;++i) {
                        XMoveWindow(dis,children[i],0,0);
                        XResizeWindow(dis,children[i],width,height);
                        XMapWindow(dis,children[i]);
                    }
                }

                break;
            default:
                fprintf(stderr,"Stack mode undefined!\n");
                break;
        }
    }
}

// Main programm
int main(int argc, char **argv) {
    Display *dis = XOpenDisplay(0);
    int quitter = 0;
    XEvent ev;

    int stack_mode = 0; // 0=vertical,1=horizontal 2=fullscreen
    int curr_win = 0;

    if(!dis) {
        fprintf(stderr,"Cannot open display!\n");
        return 1;
    }

    Window root = DefaultRootWindow(dis);

    create_shortcuts(dis,root);

    // Main loop
    while(!quitter) {
        // For the keybindings
        if(ev.type == KeyPress) {
            int i;
            for(i=0;i<sizeof key_bindings / sizeof *key_bindings;++i) {
                if(XLookupKeysym(&ev.xkey,0) == key_bindings[i].keysym) {
                    switch(key_bindings[i].funct) {
                        case k_quit:
                            quitter = 1;
                            fprintf(stdout,"Quit! Thanks for using!\n");
                            break;
                        case k_change_mode:
                            if(stack_mode == 2)
                                stack_mode = 0;
                            else
                                stack_mode++;
                            break;
                        case k_spawn_custom:
                            spawn(key_bindings[i].command);
                            break;
                        case k_close:
                            fprintf(stdout,"CACA\n");
                            Window tmp;
                            int tmp_ret;
                            XGetInputFocus(dis,&tmp,&tmp_ret);
                            break;
                        case k_next_win:
                            {
                                Window parent;
                                Window *children;
                                unsigned int n;
                                XQueryTree (dis, root, &root, &parent, &children, &n);

                                if(curr_win == n-1) {
                                    curr_win = 0;
                                }
                                else
                                    curr_win++;

                                if(n > 1) {
                                    XSetInputFocus(dis,children[curr_win],0,CurrentTime);
                                    if(stack_mode == 2)
                                        XRaiseWindow(dis,children[curr_win]);
                                }
                            }
                            break;
                        default:
                            fprintf(stderr,"Undefined keybinding");
                            break;
                    }
                }
            }
        }

        tile_windows(dis,root,stack_mode);
        XNextEvent(dis,&ev);
    }

    XCloseDisplay(dis);
    return 0;
} 
