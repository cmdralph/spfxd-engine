#include <coffee/input.h>

namespace coffee {

namespace {

template<typename Enum, std::size_t Size>
bool read_state(const std::array<bool, Size>& states, Enum value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < Size && states[index];
}

} // namespace

bool Input::key_down(key value) const noexcept { return read_state(keys_down_, value); }
bool Input::key_pressed(key value) const noexcept { return read_state(keys_pressed_, value); }
bool Input::key_released(key value) const noexcept { return read_state(keys_released_, value); }
bool Input::mouse_down(mouse_button value) const noexcept { return read_state(mouse_down_, value); }
bool Input::mouse_pressed(mouse_button value) const noexcept { return read_state(mouse_pressed_, value); }
bool Input::mouse_released(mouse_button value) const noexcept { return read_state(mouse_released_, value); }
vec2 Input::mouse_position() const noexcept { return mouse_position_; }
vec2 Input::mouse_delta() const noexcept { return mouse_delta_; }
vec2 Input::scroll_delta() const noexcept { return scroll_delta_; }

} // namespace coffee

