#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <xcb/xcb.h>

#include "event.h"
#include "types.h"

static xcb_connection_t *dpy;
static xcb_screen_t *scr;
static xcb_drawable_t root;

/* required for button press, motion and release */
static uint32_t motion_vals[3];
static xcb_window_t motion_win;
static xcb_get_geometry_reply_t *motion_geom;
static int is_motion = 0;

static void init_wm(void);
static void init_input(void);
static void run_loop(void);
static void win_add(xcb_window_t win);

void event_button_press(xcb_generic_event_t *ev) {
    xcb_button_press_event_t *e = (xcb_button_press_event_t *)ev;

    if (e->child == root) {
        return;
    }

    motion_vals[0] = XCB_STACK_MODE_ABOVE;
    motion_win = e->child;
    is_motion = 1;

    xcb_configure_window(dpy, motion_win, XCB_CONFIG_WINDOW_STACK_MODE,
        motion_vals);

    motion_geom = xcb_get_geometry_reply(dpy,
                      xcb_get_geometry(dpy, motion_win), NULL);

    /* resize */
    if (e->detail == 1) {
        xcb_warp_pointer(dpy, XCB_NONE, motion_win, 0, 0, 0, 0, 1, 1);
        motion_vals[2] = 1;

    /* move */
    } else {
        xcb_warp_pointer(dpy, XCB_NONE, motion_win, 0, 0, 0, 0, 1, 1);
        motion_vals[2] = 3;
    }

    xcb_grab_pointer(dpy, 0, root, XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
        XCB_CURRENT_TIME);

    xcb_flush(dpy);
}

void event_button_release(xcb_generic_event_t *ev) {
    xcb_ungrab_pointer(dpy, XCB_CURRENT_TIME);
    xcb_flush(dpy);
    is_motion = 0;
}

void event_configure_request(xcb_generic_event_t *ev) {
    xcb_configure_request_event_t *e = (xcb_configure_request_event_t *)ev;
	uint32_t values[7];
	int8_t i = -1;

    if (e->value_mask & XCB_CONFIG_WINDOW_X) {
        e->value_mask |= XCB_CONFIG_WINDOW_X;
        i++;
        values[i] = e->x;
    }

    if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
        e->value_mask |= XCB_CONFIG_WINDOW_Y;
        i++;
        values[i] = e->y;
    }

    if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
        e->value_mask |= XCB_CONFIG_WINDOW_WIDTH;
        i++;
        values[i] = e->width;
    }

    if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
        e->value_mask |= XCB_CONFIG_WINDOW_HEIGHT;
        i++;
        values[i] = e->height;
    }

    if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
        e->value_mask |= XCB_CONFIG_WINDOW_SIBLING;
        i++;
        values[i] = e->sibling;
    }

    if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
        e->value_mask |= XCB_CONFIG_WINDOW_STACK_MODE;
        i++;
        values[i] = e->stack_mode;
    }

    if (i != -1) {
        xcb_configure_window(dpy, e->window, e->value_mask, values);
        xcb_flush(dpy);
    }
}

void event_key_press(xcb_generic_event_t *ev) {
    xcb_key_press_event_t *e = (xcb_key_press_event_t *)ev;
}

void event_notify_create(xcb_generic_event_t *ev) {
	xcb_create_notify_event_t *e = (xcb_create_notify_event_t *)ev;
    uint32_t values[2];

    values[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
    values[1] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    xcb_change_window_attributes(dpy, e->window, XCB_CW_EVENT_MASK, values);
    xcb_map_window(dpy, e->window);
}

void event_notify_destroy(xcb_generic_event_t *ev) {
    xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *)ev;
}

void event_notify_enter(xcb_generic_event_t *ev) {
    xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *)ev;

    if (e->event == root || is_motion) {
        return;
    }

    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT,
        e->event, XCB_CURRENT_TIME);

    xcb_flush(dpy);
}

void event_notify_motion(xcb_generic_event_t *ev) {
    xcb_query_pointer_reply_t *ptr;

    ptr = xcb_query_pointer_reply(dpy, xcb_query_pointer(dpy, root), 0);

    /* move */
    if (motion_vals[2] == 1) {
        motion_geom = xcb_get_geometry_reply(dpy,
            xcb_get_geometry(dpy, motion_win), NULL);

        motion_vals[0] =
            (ptr->root_x + motion_geom->width > scr->width_in_pixels)?
            (scr->width_in_pixels - motion_geom->width):ptr->root_x;

        motion_vals[1] =
            (ptr->root_y + motion_geom->height > scr->height_in_pixels)?
            (scr->height_in_pixels - motion_geom->height):ptr->root_y;

        xcb_configure_window(dpy, motion_win, XCB_CONFIG_WINDOW_X |
            XCB_CONFIG_WINDOW_Y, motion_vals);

        xcb_flush(dpy);

    /* resize */
    } else if (motion_vals[2] == 3) {
        motion_geom = xcb_get_geometry_reply(dpy,
            xcb_get_geometry(dpy, motion_win), NULL);

        motion_vals[0] = ptr->root_x - motion_geom->x;
        motion_vals[1] = ptr->root_y - motion_geom->y;

        xcb_configure_window(dpy, motion_win, XCB_CONFIG_WINDOW_WIDTH |
            XCB_CONFIG_WINDOW_HEIGHT, motion_vals);

        xcb_flush(dpy);
    }
}

void win_add(xcb_window_t win) {

}

void init_wm(void) {
    uint32_t values[2];
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

    root = scr->root;

    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    xcb_change_window_attributes_checked(dpy, root, XCB_CW_EVENT_MASK, values);
    xcb_flush(dpy);
}

void init_input(void) {
    xcb_grab_key(dpy, 1, root, XCB_MOD_MASK_4, XCB_NO_SYMBOL,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    xcb_grab_button(dpy, 0, root, XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
        XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 1, XCB_MOD_MASK_4);

    xcb_grab_button(dpy, 0, root, XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
        XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 3, XCB_MOD_MASK_4);

    xcb_flush(dpy);
}

void run_loop(void) {
    xcb_generic_event_t *ev;

    for (;;) {
        ev = xcb_wait_for_event(dpy);

        switch (EVENT_MASK(ev->response_type)) {
            case XCB_BUTTON_PRESS: {
                break;
            }

            case XCB_BUTTON_RELEASE: {
                break;
            }

            case XCB_CONFIGURE_REQUEST: {
                break;
            }

            case XCB_KEY_PRESS: {
                break;
            }

            case XCB_CREATE_NOTIFY: {
                break;
            }

            case XCB_DESTROY_NOTIFY: {
                break;
            }

            case XCB_ENTER_NOTIFY: {
                break;
            }

            case XCB_MOTION_NOTIFY: {
                break;
            }
        }

        free(ev);
    }
}

int main(int argc, char *argv[]) {
    /* unused */
    (void) argc;
    (void) argv;

    /* we don't want sigchld */
    signal(SIGCHLD, SIG_IGN);

    init_wm();
    init_input();
    run_loop();

    return 0;
}
