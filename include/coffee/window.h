#pragma once

#include <coffee/api.h>
#include <coffee/event.h>
#include <coffee/input.h>
#include <coffee/math.h>
#include <coffee/config.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace coffee {

struct window_size {
    int width = 0;
    int height = 0;
    [[nodiscard]] constexpr bool operator==(const window_size&) const noexcept = default;
};

struct window_position {
    int x = 0;
    int y = 0;
};

enum class vsync_mode {
    off = 0,
    on = 1,
    adaptive = -1
};

enum class graphics_backend {
    opengl,
    vulkan
};

struct window_config {
    std::string title = "Coffee";
    int width = 1280;
    int height = 720;
    bool resizable = true;
    bool high_pixel_density = true;
    bool fullscreen = false;
    bool maximized = false;
    bool hidden = false;
    graphics_backend backend = graphics_backend::opengl;
    int depth_bits = 24;
    int stencil_bits = 8;
    int samples = 0;
    int opengl_major = 3;
    int opengl_minor = 3;
    bool debug_context = false;
    vsync_mode vsync = vsync_mode::on;
};

class COFFEE_API Window final {
public:
    Window(const std::string& title, int width, int height);
    explicit Window(window_config config = {});
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] graphics_backend backend() const noexcept;
    void close() noexcept;

    // Pumps SDL's process-wide event queue. Call exactly once per frame.
    void poll_events();
    [[nodiscard]] std::span<const event> events() const noexcept;
    [[nodiscard]] const Input& input() const noexcept;

    void present();
    void make_current();
    void clear(color value, bool clear_depth = true, bool clear_stencil = false);

    [[nodiscard]] window_size size() const noexcept;
    [[nodiscard]] window_size framebuffer_size() const noexcept;
    [[nodiscard]] window_position position() const noexcept;
    [[nodiscard]] float display_scale() const noexcept;

    void set_title(const std::string& title);
    [[nodiscard]] std::string title() const;
    void set_size(int width, int height);
    void set_position(int x, int y);
    void show();
    void hide();
    void minimize();
    void maximize();
    void restore();
    void set_fullscreen(bool enabled);
    [[nodiscard]] bool fullscreen() const noexcept;
    [[nodiscard]] bool visible() const noexcept;
    [[nodiscard]] bool minimized() const noexcept;
    [[nodiscard]] bool maximized() const noexcept;
    [[nodiscard]] bool focused() const noexcept;
    bool set_vsync(vsync_mode mode) noexcept;
    void set_resizable(bool enabled);
    void set_mouse_grabbed(bool enabled);
    void set_relative_mouse_mode(bool enabled);
    void set_cursor_visible(bool visible);
    void start_text_input();
    void stop_text_input();

    static void set_clipboard_text(const std::string& text);
    [[nodiscard]] static std::string clipboard_text();
    [[nodiscard]] static bool has_clipboard_text() noexcept;

    [[nodiscard]] void* native_window_handle() const noexcept;
    [[nodiscard]] void* native_gl_context() const noexcept;

    // Backend-neutral bridge used by the optional CoffeeVulkan module. These
    // methods avoid exposing SDL types or linking SDL into an application.
    [[nodiscard]] std::vector<std::string> vulkan_instance_extensions() const;
    [[nodiscard]] std::uintptr_t create_vulkan_surface(
        std::uintptr_t instance,
        const void* allocation_callbacks = nullptr) const;

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace coffee
