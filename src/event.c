#include <xcb/xcb.h>

#include "globals.h"
#include "event.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static xcb_window_t motion_win;

void (*events[XCB_NO_OPERATION])(xcb_generic_event_t *) = {
    [XCB_BUTTON_PRESS]      = event_button_press,
    [XCB_BUTTON_RELEASE]    = event_button_release,
    /* [XCB_CONFIGURE_REQUEST] = event_configure_request, */
    /* [XCB_KEY_PRESS]         = event_key_press, */
    [XCB_CREATE_NOTIFY]     = event_notify_create,
    /* [XCB_DESTROY_NOTIFY]    = event_notify_destroy, */
    [XCB_ENTER_NOTIFY]      = event_notify_enter,
    [XCB_MOTION_NOTIFY]     = event_notify_motion
};

void event_button_press(xcb_generic_event_t *ev) {
    xcb_button_press_event_t *e = (xcb_button_press_event_t *)ev;
    xcb_get_geometry_reply_t *geom;
    uint32_t value;

    if (!e->child) {
        return;
    }

    motion_win = e->child;
    value = XCB_STACK_MODE_ABOVE;

    xcb_configure_window(dpy, e->child,
        XCB_CONFIG_WINDOW_STACK_MODE, &value);

    geom = xcb_get_geometry_reply(dpy,
        xcb_get_geometry(dpy, e->child), NULL);

    xcb_warp_pointer(dpy, XCB_NONE, e->child, 0, 0, 0, 0,
        e->detail != 1 ? geom->width : 1,
        e->detail != 1 ? geom->height : 1);

    xcb_grab_pointer(dpy, 0, scr->root, XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, scr->root, XCB_NONE,
        XCB_CURRENT_TIME);
}

void event_button_release(xcb_generic_event_t *ev) {
    (void)(ev);
    xcb_ungrab_pointer(dpy, XCB_CURRENT_TIME);
    motion_win = 0;
}

/* void event_configure_request(xcb_generic_event_t *ev) { */

/* } */

/* void event_key_press(xcb_generic_event_t *ev) { */

/* } */

void event_notify_create(xcb_generic_event_t *ev) {
    xcb_create_notify_event_t *e = (xcb_create_notify_event_t *)ev;
    uint32_t value;

    value = XCB_EVENT_MASK_ENTER_WINDOW |
            XCB_EVENT_MASK_FOCUS_CHANGE |
            XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    xcb_change_window_attributes(dpy, e->window, XCB_CW_EVENT_MASK, &value);
    xcb_map_window(dpy, e->window);

    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT,
        e->window, XCB_CURRENT_TIME);
}

/* void event_notify_destroy(xcb_generic_event_t *ev) { */

/* } */

void event_notify_enter(xcb_generic_event_t *ev) {
    xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *)ev;

    if (!e->event || e->event == scr->root) {
        return;
    }

    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT,
        e->event, XCB_CURRENT_TIME);
}

void event_notify_motion(xcb_generic_event_t *ev) {
    xcb_motion_notify_event_t *e = (xcb_motion_notify_event_t *)ev;
    xcb_query_pointer_reply_t *ptr;
    xcb_get_geometry_reply_t *geom;
    uint32_t values[2];

    if (!motion_win || motion_win == scr->root) {
        return;
    }

    ptr = xcb_query_pointer_reply(dpy, xcb_query_pointer(dpy, scr->root), 0);
    geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, motion_win), NULL);

    /* move */
    if (e->state & XCB_BUTTON_MASK_1) {
        values[0] = (ptr->root_x + geom->width > scr->width_in_pixels)?
            (scr->width_in_pixels - geom->width):ptr->root_x;

        values[1] = (ptr->root_y + geom->height > scr->height_in_pixels)?
            (scr->height_in_pixels - geom->height):ptr->root_y;

        xcb_configure_window(dpy, motion_win,
            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);

    /* resize */
    } else if (e->state & XCB_BUTTON_MASK_3) {
        values[0] = MAX(10, ptr->root_x - geom->x);
        values[1] = MAX(10, ptr->root_y - geom->y);

        xcb_configure_window(dpy, motion_win,
            XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
    }
}
