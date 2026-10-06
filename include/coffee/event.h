#pragma once

#include <coffee/input.h>
#include <coffee/math.h>

#include <string>

namespace coffee {

enum class event_type {
    none,
    quit,
    window_close,
    window_resized,
    framebuffer_resized,
    window_moved,
    window_minimized,
    window_maximized,
    window_restored,
    display_scale_changed,
    focus_gained,
    focus_lost,
    key_down,
    key_up,
    text_input,
    mouse_move,
    mouse_button_down,
    mouse_button_up,
    mouse_wheel,
    mouse_enter,
    mouse_leave,
    clipboard_changed,
    file_drop,
    text_drop
};

struct event {
    event_type type = event_type::none;
    key key_code = key::unknown;
    mouse_button button = mouse_button::left;
    bool repeat = false;
    int width = 0;
    int height = 0;
    vec2 position{};
    vec2 delta{};
    std::string text{};
};

} // namespace coffee
