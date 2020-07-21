#include "globals.h"
#include "event.h"

void (*events[XCB_NO_OPERATION])(xcb_generic_event_t *) = {
    [XCB_BUTTON_PRESS]      = event_button_press,
    [XCB_BUTTON_RELEASE]    = event_button_release,
    [XCB_CONFIGURE_REQUEST] = event_configure_request,
    [XCB_KEY_PRESS]         = event_key_press,
    [XCB_CREATE_NOTIFY]     = event_notify_create,
    [XCB_DESTROY_NOTIFY]    = event_notify_destroy,
    [XCB_ENTER_NOTIFY]      = event_notify_enter,
    [XCB_MOTION_NOTIFY]     = event_notify_motion
};

void event_button_press(xcb_generic_event_t *ev) {

}

void event_button_release(xcb_generic_event_t *ev) {
    xcb_ungrab_pointer(dpy, XCB_CURRENT_TIME);
    xcb_flush(dpy);
}

void event_configure_request(xcb_generic_event_t *ev) {

}

void event_key_press(xcb_generic_event_t *ev) {

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

}

void event_notify_enter(xcb_generic_event_t *ev) {
    xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *)ev;

    if (e->event == scr->root) {
        return;
    }

    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT,
        e->event, XCB_CURRENT_TIME);

    xcb_flush(dpy);
}

void event_notify_motion(xcb_generic_event_t *ev) {

}
