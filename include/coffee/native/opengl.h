#pragma once

#include <coffee/api.h>

namespace coffee::native {

using opengl_proc_type = void (*)();

// Returns a function pointer for the window system's current OpenGL driver.
// Applications that need raw OpenGL can use this with their own loader
// without making Coffee's private GLAD state part of the DLL ABI.
[[nodiscard]] COFFEE_API opengl_proc_type opengl_proc_address(const char* name) noexcept;

template<typename Function>
[[nodiscard]] Function opengl_proc(const char* name) noexcept {
    return reinterpret_cast<Function>(opengl_proc_address(name));
}

} // namespace coffee::native
