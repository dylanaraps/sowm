#include <xcb/xcb.h>

struct wconf {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    xcb_window_t sibling;
    uint8_t stack_mode;
};
