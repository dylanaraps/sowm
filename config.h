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

#define MOD Mod4Mask

const char* menu[]    = {"dmenu_run",      NULL};
const char* term[]    = {"st",             NULL};
const char* scrot[]   = {"scr",            NULL};
const char* briup[]   = {"bri", "10", "+", NULL};
const char* bridown[] = {"bri", "10", "-", NULL};
const char* voldown[] = {"amixer", "sset", "Master", "5\%-",        NULL};
const char* volup[]   = {"amixer", "sset", "Master", "5\%+",        NULL};
const char* volmute[] = {"amixer", "sset", "Master", "toggle",      NULL};
const char* colors[]  = {"bud", "/home/goldie/Pictures/Wallpapers", NULL};

#define DESKTOP(K,N) \
    {  MOD,           K, ws_go,     {.i = N}}, \
    {  MOD|ShiftMask, K, win_to_ws, {.i = N}},

static struct key keys[] = {
    {MOD,      XK_q,   win_kill,   {NULL}},
    {MOD,      XK_c,   win_center, {NULL}},
    {MOD,      XK_f,   win_fs,     {NULL}},
    {Mod1Mask, XK_Tab, win_next,   {NULL}},

    {MOD, XK_d,      run, {.com = menu}},
    {MOD, XK_w,      run, {.com = colors}},
    {MOD, XK_p,      run, {.com = scrot}},
    {MOD, XK_Return, run, {.com = term}},

    {0,   XF86XK_AudioLowerVolume,  run, {.com = voldown}},
    {0,   XF86XK_AudioRaiseVolume,  run, {.com = volup}},
    {0,   XF86XK_AudioMute,         run, {.com = volmute}},
    {0,   XF86XK_MonBrightnessUp,   run, {.com = briup}},
    {0,   XF86XK_MonBrightnessDown, run, {.com = bridown}},

     DESKTOP( XK_0, 0)
     DESKTOP( XK_1, 1)
     DESKTOP( XK_2, 2)
     DESKTOP( XK_3, 3)
     DESKTOP( XK_4, 4)
     DESKTOP( XK_5, 5)
     DESKTOP( XK_6, 6)
     DESKTOP( XK_7, 7)
     DESKTOP( XK_8, 8)
     DESKTOP( XK_9, 9)
};

#endif

