#pragma once

#include <coffee/api.h>

#include <coffee/math.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace coffee {

namespace detail { struct input_access; }

// Physical keyboard positions. Values intentionally follow the USB keyboard
// usage IDs used by SDL scancodes, so input does not depend on the active layout.
enum class key : std::uint16_t {
    unknown = 0,
    a = 4, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
    num_1 = 30, num_2, num_3, num_4, num_5, num_6, num_7, num_8, num_9, num_0,
    enter = 40, escape, backspace, tab, space,
    minus, equals, left_bracket, right_bracket, backslash,
    semicolon = 51, apostrophe, grave, comma, period, slash, caps_lock,
    f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12,
    print_screen, scroll_lock, pause, insert, home, page_up, delete_key, end, page_down,
    right, left, down, up,
    num_lock, keypad_divide, keypad_multiply, keypad_minus, keypad_plus, keypad_enter,
    keypad_1, keypad_2, keypad_3, keypad_4, keypad_5, keypad_6, keypad_7, keypad_8,
    keypad_9, keypad_0, keypad_period,
    application = 101,
    f13 = 104, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    left_control = 224, left_shift, left_alt, left_super,
    right_control, right_shift, right_alt, right_super
};

enum class mouse_button : std::uint8_t {
    left = 1,
    middle = 2,
    right = 3,
    side_back = 4,
    side_forward = 5
};

class COFFEE_API Input final {
public:
    [[nodiscard]] bool key_down(key value) const noexcept;
    [[nodiscard]] bool key_pressed(key value) const noexcept;
    [[nodiscard]] bool key_released(key value) const noexcept;

    [[nodiscard]] bool mouse_down(mouse_button value) const noexcept;
    [[nodiscard]] bool mouse_pressed(mouse_button value) const noexcept;
    [[nodiscard]] bool mouse_released(mouse_button value) const noexcept;

    [[nodiscard]] vec2 mouse_position() const noexcept;
    [[nodiscard]] vec2 mouse_delta() const noexcept;
    [[nodiscard]] vec2 scroll_delta() const noexcept;

private:
    friend struct detail::input_access;
    static constexpr std::size_t key_capacity = 256;
    static constexpr std::size_t mouse_capacity = 8;

    std::array<bool, key_capacity> keys_down_{};
    std::array<bool, key_capacity> keys_pressed_{};
    std::array<bool, key_capacity> keys_released_{};
    std::array<bool, mouse_capacity> mouse_down_{};
    std::array<bool, mouse_capacity> mouse_pressed_{};
    std::array<bool, mouse_capacity> mouse_released_{};
    vec2 mouse_position_{};
    vec2 mouse_delta_{};
    vec2 scroll_delta_{};
};

} // namespace coffee
