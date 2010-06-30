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

#ifndef CONFIG_H
#define CONFIG_H

// Mod (Mod1 == alt) and master size
#define MOD             Mod1Mask
#define MASTER_SIZE     0.6

// Colors
#define FOCUS           "rgb:bc/57/66"
#define UNFOCUS         "rgb:88/88/88"

const char* dmenucmd[] = {"dmenu_run",NULL};
const char* urxvtcmd[] = {"urxvt",NULL};
const char* lockcmd[]  = {"slock",NULL};
const char* next[]     = { "ncmpcpp","next",NULL};
const char* prev[]     = { "ncmpcpp","prev",NULL};
const char* toggle[]   = { "ncmpcpp","toggle",NULL };
const char* voldown[]  = { "amixer","set","PCM","5\%-",NULL};
const char* volup[]    = { "amixer","set","PCM","5\%+",NULL};

// Shortcuts
struct key keys[] = {
    // MOD              KEY                         FUNCTION        ARGS
    {  MOD,             XK_h,                       decrease,       NULL},
    {  MOD,             XK_l,                       increase,       NULL},
    {  MOD,             XK_x,                       kill_client,    NULL},
    {  MOD,             XK_j,                       next_win,       NULL},
    {  MOD,             XK_Tab,                     next_win,       NULL},
    {  MOD,             XK_k,                       prev_win,       NULL},
    {  MOD,             XK_q,                       quit,           NULL},
    {  MOD,             XK_Return,                  swap_master,    NULL},
    {  MOD,             XK_space,                   switch_mode,    NULL},
    {  MOD,             XK_c,                       spawn,          lockcmd},
    {  0,               XF86XK_AudioNext,           spawn,          next},
    {  0,               XF86XK_AudioPrev,           spawn,          prev},
    {  0,               XF86XK_AudioPlay,           spawn,          toggle},
    {  0,               XF86XK_AudioLowerVolume,    spawn,          voldown},
    {  0,               XF86XK_AudioRaiseVolume,    spawn,          volup},
    {  MOD,             XK_p,                       spawn,          dmenucmd},
    {  MOD|ShiftMask,   XK_Return,                  spawn,          urxvtcmd}
};

#endif

