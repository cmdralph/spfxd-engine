#pragma once

#include <coffee/window.h>

struct SDL_Window;

namespace coffee::native {

[[nodiscard]] inline SDL_Window* sdl_window(const Window& window) noexcept {
    return static_cast<SDL_Window*>(window.native_window_handle());
}

[[nodiscard]] inline void* sdl_gl_context(const Window& window) noexcept {
    return window.native_gl_context();
}

} // namespace coffee::native
