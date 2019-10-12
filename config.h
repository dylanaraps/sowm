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
const char* voldown[] = {"amixer", "sset", "Master", "5%-",        NULL};
const char* volup[]   = {"amixer", "sset", "Master", "5%+",        NULL};
const char* volmute[] = {"amixer", "sset", "Master", "toggle",      NULL};
const char* colors[]  = {"bud", "/home/goldie/Pictures/Wallpapers", NULL};

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

    {MOD,           XK_1, ws_go,     {.i = 1}},
    {MOD|ShiftMask, XK_1, win_to_ws, {.i = 1}},
    {MOD,           XK_2, ws_go,     {.i = 2}},
    {MOD|ShiftMask, XK_2, win_to_ws, {.i = 2}},
    {MOD,           XK_3, ws_go,     {.i = 3}},
    {MOD|ShiftMask, XK_3, win_to_ws, {.i = 3}},
    {MOD,           XK_4, ws_go,     {.i = 4}},
    {MOD|ShiftMask, XK_4, win_to_ws, {.i = 4}},
    {MOD,           XK_5, ws_go,     {.i = 5}},
    {MOD|ShiftMask, XK_5, win_to_ws, {.i = 5}},
    {MOD,           XK_6, ws_go,     {.i = 6}},
    {MOD|ShiftMask, XK_6, win_to_ws, {.i = 6}},
};

#endif

