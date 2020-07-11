#include <xcb/xcb.h>

#define EVENT_MASK(e) (((e) & ~0x80))

void event_button_press(xcb_generic_event_t *ev);
void event_button_release(xcb_generic_event_t *ev);
void event_configure_request(xcb_generic_event_t *ev);
void event_key_press(xcb_generic_event_t *ev);
void event_notify_create(xcb_generic_event_t *ev);
void event_notify_destroy(xcb_generic_event_t *ev);
void event_notify_enter(xcb_generic_event_t *ev);
void event_notify_motion(xcb_generic_event_t *ev);

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
