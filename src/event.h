#include <xcb/xcb.h>

void event_button_press(xcb_generic_event_t *ev);
void event_button_release(xcb_generic_event_t *ev);
/* void event_configure_request(xcb_generic_event_t *ev); */
/* void event_key_press(xcb_generic_event_t *ev); */
void event_notify_create(xcb_generic_event_t *ev);
/* void event_notify_destroy(xcb_generic_event_t *ev); */
void event_notify_enter(xcb_generic_event_t *ev);
void event_notify_motion(xcb_generic_event_t *ev);

extern void (*events[XCB_NO_OPERATION])(xcb_generic_event_t *);
