#define _POSIX_C_SOURCE 200809L
#include <xcb/xcb.h>

struct wconf {
	int16_t x;
    int16_t y;
	uint16_t width;
    uint16_t height;
	uint8_t stack_mode;
	xcb_window_t sibling;
};

xcb_connection_t *dpy;
xcb_screen_t *scr;
xcb_drawable_t root;

uint32_t ev_vals[3];

/* xcb event with the largest value */
#define XCB_LAST_EVENT XCB_GET_MODIFIER_MAPPING
#define EVENT_MASK(e) (((e) & ~0x80))

void init_wm(void);
void init_input(void);
void run_loop(void);

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

void win_add(xcb_window_t win);
