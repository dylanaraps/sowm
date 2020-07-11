#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <xcb/xcb.h>

#include "sowm.h"

void event_button_press(xcb_generic_event_t *ev) {
    xcb_button_press_event_t *e = (xcb_button_press_event_t *)ev;

    ev_vals[0] = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(dpy, e->child, XCB_CONFIG_WINDOW_STACK_MODE, ev_vals);

    xcb_grab_pointer(dpy, 0, root, XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
        XCB_CURRENT_TIME);
}

void event_button_release(xcb_generic_event_t *ev) {
    xcb_ungrab_pointer(dpy, XCB_CURRENT_TIME);
}

void event_configure_request(xcb_generic_event_t *ev) {
    xcb_configure_request_event_t *e = (xcb_configure_request_event_t *)ev;
	uint32_t values[7];
	int8_t i = -1;

	if (e->value_mask & XCB_CONFIG_WINDOW_X) {
		e->value_mask |= XCB_CONFIG_WINDOW_X;
		values[++i] = e->x;
	}

	if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
		e->value_mask |= XCB_CONFIG_WINDOW_Y;
		values[++i] = e->y;
	}

	if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
		e->value_mask |= XCB_CONFIG_WINDOW_WIDTH;
		values[++i] = e->width;
	}

	if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
		e->value_mask |= XCB_CONFIG_WINDOW_HEIGHT;
		values[++i] = e->height;
	}

	if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
		e->value_mask |= XCB_CONFIG_WINDOW_SIBLING;
		values[++i] = e->sibling;
	}

	if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
		e->value_mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		values[++i] = e->stack_mode;
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

    ev_vals[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
    ev_vals[1] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    xcb_change_window_attributes(dpy, e->window, XCB_CW_EVENT_MASK, ev_vals);
    xcb_map_window(dpy, e->window);
    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_PARENT,
        e->window, XCB_CURRENT_TIME);
}

void event_notify_destroy(xcb_generic_event_t *ev) {
    xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *)ev;
}

void event_notify_enter(xcb_generic_event_t *ev) {
    xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *)ev;

    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_PARENT,
        e->event, XCB_CURRENT_TIME);
}

void event_notify_motion(xcb_generic_event_t *ev) {
    xcb_motion_notify_event_t *e = (xcb_motion_notify_event_t *)ev;
}

void win_add(xcb_window_t win) {

}

void init_wm(void) {
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

    ev_vals[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    xcb_change_window_attributes_checked(dpy, root, XCB_CW_EVENT_MASK, ev_vals);
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

        if (events[EVENT_MASK(ev->response_type)]) {
            events[EVENT_MASK(ev->response_type)](ev);
            xcb_flush(dpy);
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
