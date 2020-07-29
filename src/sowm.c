#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <xcb/xcb.h>

#include "event.h"
#include "vec.h"
#include "globals.h"
#include "config.h"

static void init_wm(void);
static void init_input(void);
static void init_desktops(void);

xcb_connection_t *dpy;
xcb_screen_t *scr;

struct desktop *desktops;
int current_desktop = 0;

static void init_wm() {
    uint32_t values;

    dpy = xcb_connect(NULL, NULL);

    if (xcb_connection_has_error(dpy)) {
        printf("error: Failed to start sowm\n");
        exit(1);
    }

    scr = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;

    if (!scr) {
        printf("error: Failed to assign screen number\n");
        xcb_disconnect(dpy);
        exit(1);
    }

    values = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    xcb_change_window_attributes_checked(dpy, scr->root,
        XCB_CW_EVENT_MASK, &values);

    xcb_flush(dpy);
}

static void init_input() {
    xcb_grab_key(dpy, 1, scr->root, SOWM_MOD, XCB_NO_SYMBOL,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    xcb_grab_button(dpy, 0, scr->root, XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
        XCB_GRAB_MODE_ASYNC, scr->root, XCB_NONE, 1, SOWM_MOD);

    xcb_grab_button(dpy, 0, scr->root, XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
        XCB_GRAB_MODE_ASYNC, scr->root, XCB_NONE, 3, SOWM_MOD);

    xcb_flush(dpy);
}

static void init_desktops() {
    struct desktop new = {0};

    for (int i = 0; i < SOWM_NUM_DESKTOPS; i++) {
        new.num = i;
        vec_push_back(desktops, new);
    }

    /* todo finish usage of desktops */
    vec_free(desktops);
}

int main(int argc, char **argv) {
    xcb_generic_event_t *ev;
    unsigned int ev_type;

    /* unused */
    (void) argc;
    (void) argv;

    /* we don't want sigchld */
    signal(SIGCHLD, SIG_IGN);

    init_wm();
    init_input();
    init_desktops();

    while ((ev = xcb_wait_for_event(dpy))) {
        ev_type = ev->response_type & ~0x80;

        if (events[ev_type]) {
            events[ev_type](ev);
            xcb_flush(dpy);
        }

        free(ev);
    }

    return 0;
}
