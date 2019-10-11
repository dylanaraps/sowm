 /*
 *   /\___/\
 *  ( o   o )  Made by cat...
 *  (  =^=  )
 *  (        )            ... for cat!
 *  (         )
 *  (          ))))))________________
 *  __________________
 */

#ifndef CONFIG_H
#define CONFIG_H

// Mod (Mod1 == alt) and master size
#define MOD             Mod4Mask
#define MASTER_SIZE     0.5

// Colors
#define FOCUS           "rgb:bc/57/66"
#define UNFOCUS         "rgb:88/88/88"

const char* dmenucmd[] = {"dmenu_run",NULL};
const char* urxvtcmd[] = {"st",NULL};
const char* voldown[]  = {"amixer","sset","Master","5\%-",NULL};
const char* volup[]    = {"amixer","sset","Master","5\%+",NULL};

// Avoid multiple paste
#define DESKTOPCHANGE(K,N) \
    {  MOD,             K,                          change_desktop, {.i = N}}, \
    {  MOD|ShiftMask,   K,                          client_to_desktop, {.i = N}},

// Shortcuts
static struct key keys[] = {
    // MOD              KEY                         FUNCTION        ARGS
    {  MOD,             XK_q,                       kill_client,    {NULL}},
    {  MOD,             XK_c,                       center_window,  {NULL}},
    {  MOD,             XK_f,                       fullscreen_toggle,  {NULL}},
    {  Mod1Mask,        XK_Tab,                     next_win,       {NULL}},
    {  0,               XF86XK_AudioLowerVolume,    spawn,          {.com = voldown}},
    {  0,               XF86XK_AudioRaiseVolume,    spawn,          {.com = volup}},
    {  MOD,             XK_d,                       spawn,          {.com = dmenucmd}},
    {  MOD,             XK_Return,                  spawn,          {.com = urxvtcmd}},
       DESKTOPCHANGE(   XK_0,                                       0)
       DESKTOPCHANGE(   XK_1,                                       1)
       DESKTOPCHANGE(   XK_2,                                       2)
       DESKTOPCHANGE(   XK_3,                                       3)
       DESKTOPCHANGE(   XK_4,                                       4)
       DESKTOPCHANGE(   XK_5,                                       5)
       DESKTOPCHANGE(   XK_6,                                       6)
       DESKTOPCHANGE(   XK_7,                                       7)
       DESKTOPCHANGE(   XK_8,                                       8)
       DESKTOPCHANGE(   XK_9,                                       9)
};

#endif

