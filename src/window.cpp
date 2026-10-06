#include <coffee/window.h>
#include <coffee/error.h>
#include <coffee/native/opengl.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <glad/gl.h>

#include <algorithm>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#if !SDL_VERSION_ATLEAST(3, 2, 0)
#error "Coffee requires SDL 3.2.0 or newer."
#endif

namespace coffee {

namespace detail {

struct input_access {
    static void begin_frame(Input& input) noexcept {
        input.keys_pressed_.fill(false);
        input.keys_released_.fill(false);
        input.mouse_pressed_.fill(false);
        input.mouse_released_.fill(false);
        input.mouse_delta_ = {};
        input.scroll_delta_ = {};
    }

    static void key_changed(Input& input, int code, bool down, bool repeat) noexcept {
        if (code < 0 || static_cast<std::size_t>(code) >= Input::key_capacity) {
            return;
        }
        const auto index = static_cast<std::size_t>(code);
        if (down) {
            if (!input.keys_down_[index] && !repeat) input.keys_pressed_[index] = true;
            input.keys_down_[index] = true;
        } else {
            if (input.keys_down_[index]) input.keys_released_[index] = true;
            input.keys_down_[index] = false;
        }
    }

    static void mouse_changed(Input& input, int code, bool down) noexcept {
        if (code < 0 || static_cast<std::size_t>(code) >= Input::mouse_capacity) {
            return;
        }
        const auto index = static_cast<std::size_t>(code);
        if (down) {
            if (!input.mouse_down_[index]) input.mouse_pressed_[index] = true;
            input.mouse_down_[index] = true;
        } else {
            if (input.mouse_down_[index]) input.mouse_released_[index] = true;
            input.mouse_down_[index] = false;
        }
    }

    static void mouse_moved(Input& input, vec2 position, vec2 delta) noexcept {
        input.mouse_position_ = position;
        input.mouse_delta_ += delta;
    }

    static void scrolled(Input& input, vec2 delta) noexcept { input.scroll_delta_ += delta; }
    static void clear(Input& input) noexcept {
        input.keys_down_.fill(false);
        input.mouse_down_.fill(false);
    }
};

} // namespace detail

namespace {

[[noreturn]] void throw_sdl(error_code code, const std::string& operation) {
    throw error(code, operation + ": " + SDL_GetError());
}

void require_sdl(bool result, error_code code, const std::string& operation) {
    if (!result) throw_sdl(code, operation);
}

class event_dispatcher final {
public:
    struct handlers {
        std::function<void()> begin_frame;
        std::function<void(const SDL_Event&)> handle;
    };

    static event_dispatcher& instance() {
        static event_dispatcher value;
        return value;
    }

    void add(SDL_WindowID id, handlers callbacks) { handlers_[id] = std::move(callbacks); }
    void remove(SDL_WindowID id) { handlers_.erase(id); }

    void pump() {
        for (auto& [id, callbacks] : handlers_) {
            static_cast<void>(id);
            callbacks.begin_frame();
        }

        SDL_Event native_event{};
        while (SDL_PollEvent(&native_event)) {
            if (native_event.type == SDL_EVENT_QUIT ||
                native_event.type == SDL_EVENT_CLIPBOARD_UPDATE) {
                for (auto& [id, callbacks] : handlers_) {
                    static_cast<void>(id);
                    callbacks.handle(native_event);
                }
                continue;
            }

            const SDL_WindowID id = window_id(native_event);
            if (const auto iterator = handlers_.find(id); iterator != handlers_.end()) {
                iterator->second.handle(native_event);
            }
        }
    }

private:
    static SDL_WindowID window_id(const SDL_Event& value) noexcept {
        switch (value.type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: return value.key.windowID;
            case SDL_EVENT_TEXT_INPUT: return value.text.windowID;
            case SDL_EVENT_MOUSE_MOTION: return value.motion.windowID;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: return value.button.windowID;
            case SDL_EVENT_MOUSE_WHEEL: return value.wheel.windowID;
            case SDL_EVENT_DROP_FILE:
            case SDL_EVENT_DROP_TEXT: return value.drop.windowID;
            default:
                if (value.type >= SDL_EVENT_WINDOW_FIRST && value.type <= SDL_EVENT_WINDOW_LAST) {
                    return value.window.windowID;
                }
                return 0;
        }
    }

    std::unordered_map<SDL_WindowID, handlers> handlers_;
};

std::mutex glad_mutex;
bool glad_loaded = false;

template<typename Handle>
[[nodiscard]] Handle handle_from_uintptr(std::uintptr_t value) noexcept {
    if constexpr (std::is_pointer_v<Handle>) {
        return reinterpret_cast<Handle>(value);
    } else {
        return static_cast<Handle>(value);
    }
}

template<typename Handle>
[[nodiscard]] std::uintptr_t handle_to_uintptr(Handle value) noexcept {
    if constexpr (std::is_pointer_v<Handle>) {
        return reinterpret_cast<std::uintptr_t>(value);
    } else {
        return static_cast<std::uintptr_t>(value);
    }
}

} // namespace

struct Window::impl {
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    SDL_WindowID id = 0;
    bool open = true;
    bool video_initialized = false;
    graphics_backend graphics = graphics_backend::opengl;
    Input input_state{};
    std::vector<event> frame_events{};

    ~impl() {
        if (id != 0) event_dispatcher::instance().remove(id);
        if (context != nullptr) SDL_GL_DestroyContext(context);
        if (window != nullptr) SDL_DestroyWindow(window);
        if (video_initialized) SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    void begin_frame() {
        detail::input_access::begin_frame(input_state);
        frame_events.clear();
    }

    void push(event value) { frame_events.push_back(std::move(value)); }

    void handle(const SDL_Event& value) {
        event translated{};
        switch (value.type) {
            case SDL_EVENT_QUIT:
                open = false;
                translated.type = event_type::quit;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                open = false;
                translated.type = event_type::window_close;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                translated.type = event_type::window_resized;
                translated.width = value.window.data1;
                translated.height = value.window.data2;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                translated.type = event_type::framebuffer_resized;
                translated.width = value.window.data1;
                translated.height = value.window.data2;
                if (graphics == graphics_backend::opengl && context != nullptr) {
                    SDL_GL_MakeCurrent(window, context);
                    glViewport(0, 0, translated.width, translated.height);
                }
                break;
            case SDL_EVENT_WINDOW_MOVED:
                translated.type = event_type::window_moved;
                translated.position = {
                    static_cast<float>(value.window.data1),
                    static_cast<float>(value.window.data2)};
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                translated.type = event_type::window_minimized;
                break;
            case SDL_EVENT_WINDOW_MAXIMIZED:
                translated.type = event_type::window_maximized;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                translated.type = event_type::window_restored;
                break;
            case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
                translated.type = event_type::display_scale_changed;
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                translated.type = event_type::focus_gained;
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                translated.type = event_type::focus_lost;
                detail::input_access::clear(input_state);
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                const bool down = value.type == SDL_EVENT_KEY_DOWN;
                detail::input_access::key_changed(
                    input_state, static_cast<int>(value.key.scancode), down, value.key.repeat);
                translated.type = down ? event_type::key_down : event_type::key_up;
                translated.key_code = static_cast<key>(value.key.scancode);
                translated.repeat = value.key.repeat;
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
                translated.type = event_type::text_input;
                translated.text = value.text.text != nullptr ? value.text.text : "";
                break;
            case SDL_EVENT_MOUSE_MOTION:
                translated.type = event_type::mouse_move;
                translated.position = {value.motion.x, value.motion.y};
                translated.delta = {value.motion.xrel, value.motion.yrel};
                detail::input_access::mouse_moved(input_state, translated.position, translated.delta);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                const bool down = value.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                detail::input_access::mouse_changed(input_state, value.button.button, down);
                translated.type = down ? event_type::mouse_button_down : event_type::mouse_button_up;
                translated.button = static_cast<mouse_button>(value.button.button);
                translated.position = {value.button.x, value.button.y};
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
                translated.type = event_type::mouse_wheel;
                translated.delta = {value.wheel.x, value.wheel.y};
                if (value.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) translated.delta = -translated.delta;
                detail::input_access::scrolled(input_state, translated.delta);
                break;
            case SDL_EVENT_WINDOW_MOUSE_ENTER:
                translated.type = event_type::mouse_enter;
                break;
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                translated.type = event_type::mouse_leave;
                break;
            case SDL_EVENT_CLIPBOARD_UPDATE:
                translated.type = event_type::clipboard_changed;
                break;
            case SDL_EVENT_DROP_FILE:
                translated.type = event_type::file_drop;
                translated.text = value.drop.data != nullptr ? value.drop.data : "";
                translated.position = {value.drop.x, value.drop.y};
                break;
            case SDL_EVENT_DROP_TEXT:
                translated.type = event_type::text_drop;
                translated.text = value.drop.data != nullptr ? value.drop.data : "";
                translated.position = {value.drop.x, value.drop.y};
                break;
            default:
                return;
        }
        push(std::move(translated));
    }
};

Window::Window(const std::string& title, int width, int height)
    : Window(window_config{.title = title, .width = width, .height = height}) {}

Window::Window(window_config config) : impl_(std::make_unique<impl>()) {
    if (config.width <= 0 || config.height <= 0) {
        throw error(error_code::invalid_argument, "Window dimensions must be greater than zero.");
    }
    if (config.samples < 0) {
        throw error(error_code::invalid_argument, "Multisample count cannot be negative.");
    }

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        throw_sdl(error_code::platform_initialization, "Failed to initialize SDL video");
    }
    impl_->video_initialized = true;

    impl_->graphics = config.backend;

    const auto set_attribute = [](SDL_GLAttr attribute, int value, const char* name) {
        require_sdl(SDL_GL_SetAttribute(attribute, value), error_code::graphics_initialization,
                    std::string("Failed to set ") + name);
    };
    if (config.backend == graphics_backend::opengl) {
        set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, config.opengl_major, "OpenGL major version");
        set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, config.opengl_minor, "OpenGL minor version");
        set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE, "OpenGL core profile");
        set_attribute(SDL_GL_DOUBLEBUFFER, 1, "double buffering");
        set_attribute(SDL_GL_DEPTH_SIZE, config.depth_bits, "depth buffer size");
        set_attribute(SDL_GL_STENCIL_SIZE, config.stencil_bits, "stencil buffer size");
        int context_flags = config.debug_context ? SDL_GL_CONTEXT_DEBUG_FLAG : 0;
#if defined(__APPLE__)
        context_flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#endif
        if (context_flags != 0) set_attribute(SDL_GL_CONTEXT_FLAGS, context_flags, "OpenGL context flags");
        if (config.samples > 0) {
            set_attribute(SDL_GL_MULTISAMPLEBUFFERS, 1, "multisample buffer");
            set_attribute(SDL_GL_MULTISAMPLESAMPLES, config.samples, "multisample count");
        }
    }

    SDL_WindowFlags flags = config.backend == graphics_backend::opengl ? SDL_WINDOW_OPENGL : SDL_WINDOW_VULKAN;
    if (config.resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (config.high_pixel_density) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (config.maximized) flags |= SDL_WINDOW_MAXIMIZED;
    if (config.hidden) flags |= SDL_WINDOW_HIDDEN;

    impl_->window = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
    if (impl_->window == nullptr) throw_sdl(error_code::window_creation, "Failed to create window");

    if (config.backend == graphics_backend::opengl) {
        impl_->context = SDL_GL_CreateContext(impl_->window);
        if (impl_->context == nullptr) {
            throw_sdl(error_code::graphics_initialization, "Failed to create OpenGL context");
        }
        make_current();
        {
            std::scoped_lock lock(glad_mutex);
            if (!glad_loaded) {
                if (gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)) == 0) {
                    throw error(error_code::graphics_initialization, "Failed to load OpenGL through GLAD.");
                }
                glad_loaded = true;
            }
        }
    }

    impl_->id = SDL_GetWindowID(impl_->window);
    if (impl_->id == 0) throw_sdl(error_code::window_creation, "Failed to query window ID");

    event_dispatcher::instance().add(impl_->id, {
        [instance = impl_.get()] { instance->begin_frame(); },
        [instance = impl_.get()](const SDL_Event& value) { instance->handle(value); }
    });

    if (config.backend == graphics_backend::opengl) {
        const auto pixels = framebuffer_size();
        glViewport(0, 0, pixels.width, pixels.height);
        set_vsync(config.vsync);
    }
}

Window::~Window() = default;
Window::Window(Window&& other) noexcept = default;
Window& Window::operator=(Window&& other) noexcept = default;

bool Window::is_open() const noexcept { return impl_ != nullptr && impl_->open; }
graphics_backend Window::backend() const noexcept {
    return impl_ != nullptr ? impl_->graphics : graphics_backend::opengl;
}
void Window::close() noexcept { if (impl_ != nullptr) impl_->open = false; }
void Window::poll_events() { event_dispatcher::instance().pump(); }

std::span<const event> Window::events() const noexcept {
    return impl_ != nullptr ? std::span<const event>(impl_->frame_events) : std::span<const event>{};
}

const Input& Window::input() const noexcept {
    static const Input empty{};
    return impl_ != nullptr ? impl_->input_state : empty;
}

void Window::present() {
    if (backend() != graphics_backend::opengl) {
        throw error(error_code::invalid_operation, "Vulkan frames are presented through VulkanContext::end_frame().");
    }
    if (impl_ == nullptr || impl_->window == nullptr || impl_->context == nullptr) {
        throw error(error_code::invalid_operation, "Cannot present an invalid window.");
    }
    make_current();
    if (!SDL_GL_SwapWindow(impl_->window)) {
        throw_sdl(error_code::invalid_operation, "Failed to present window");
    }
}

void Window::make_current() {
    if (backend() != graphics_backend::opengl) {
        throw error(error_code::invalid_operation, "A Vulkan window has no OpenGL context to activate.");
    }
    if (impl_ == nullptr || impl_->window == nullptr || impl_->context == nullptr) {
        throw error(error_code::invalid_operation, "Cannot activate an invalid OpenGL context.");
    }
    require_sdl(SDL_GL_MakeCurrent(impl_->window, impl_->context),
                error_code::graphics_initialization, "Failed to activate OpenGL context");
}

void Window::clear(color value, bool clear_depth, bool clear_stencil) {
    if (backend() != graphics_backend::opengl) {
        throw error(error_code::invalid_operation, "Vulkan clear operations are recorded by VulkanContext::begin_frame().");
    }
    make_current();
    glClearColor(value.x, value.y, value.z, value.w);
    GLbitfield mask = GL_COLOR_BUFFER_BIT;
    if (clear_depth) mask |= GL_DEPTH_BUFFER_BIT;
    if (clear_stencil) mask |= GL_STENCIL_BUFFER_BIT;
    glClear(mask);
}

window_size Window::size() const noexcept {
    window_size result{};
    if (impl_ != nullptr && impl_->window != nullptr) SDL_GetWindowSize(impl_->window, &result.width, &result.height);
    return result;
}

window_size Window::framebuffer_size() const noexcept {
    window_size result{};
    if (impl_ != nullptr && impl_->window != nullptr) SDL_GetWindowSizeInPixels(impl_->window, &result.width, &result.height);
    return result;
}

window_position Window::position() const noexcept {
    window_position result{};
    if (impl_ != nullptr && impl_->window != nullptr) SDL_GetWindowPosition(impl_->window, &result.x, &result.y);
    return result;
}

float Window::display_scale() const noexcept {
    return impl_ != nullptr && impl_->window != nullptr ? SDL_GetWindowDisplayScale(impl_->window) : 1.0f;
}

void Window::set_title(const std::string& title_value) {
    require_sdl(SDL_SetWindowTitle(impl_->window, title_value.c_str()), error_code::window_creation,
                "Failed to set window title");
}

std::string Window::title() const {
    if (impl_ == nullptr || impl_->window == nullptr) return {};
    const char* value = SDL_GetWindowTitle(impl_->window);
    return value != nullptr ? value : "";
}

void Window::set_size(int width, int height) {
    if (width <= 0 || height <= 0) throw error(error_code::invalid_argument, "Window size must be positive.");
    require_sdl(SDL_SetWindowSize(impl_->window, width, height), error_code::window_creation,
                "Failed to resize window");
}

void Window::set_position(int x, int y) {
    require_sdl(SDL_SetWindowPosition(impl_->window, x, y), error_code::window_creation,
                "Failed to move window");
}

void Window::show() {
    require_sdl(SDL_ShowWindow(impl_->window), error_code::window_creation,
                "Failed to show window");
}

void Window::hide() {
    require_sdl(SDL_HideWindow(impl_->window), error_code::window_creation,
                "Failed to hide window");
}

void Window::minimize() {
    require_sdl(SDL_MinimizeWindow(impl_->window), error_code::window_creation,
                "Failed to minimize window");
}

void Window::maximize() {
    require_sdl(SDL_MaximizeWindow(impl_->window), error_code::window_creation,
                "Failed to maximize window");
}

void Window::restore() {
    require_sdl(SDL_RestoreWindow(impl_->window), error_code::window_creation,
                "Failed to restore window");
}

void Window::set_fullscreen(bool enabled) {
    require_sdl(SDL_SetWindowFullscreen(impl_->window, enabled), error_code::window_creation,
                "Failed to change fullscreen state");
}

bool Window::fullscreen() const noexcept {
    return impl_ != nullptr && impl_->window != nullptr &&
           (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_FULLSCREEN) != 0;
}

bool Window::visible() const noexcept {
    return impl_ != nullptr && impl_->window != nullptr &&
           (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_HIDDEN) == 0;
}

bool Window::minimized() const noexcept {
    return impl_ != nullptr && impl_->window != nullptr &&
           (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_MINIMIZED) != 0;
}

bool Window::maximized() const noexcept {
    return impl_ != nullptr && impl_->window != nullptr &&
           (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_MAXIMIZED) != 0;
}

bool Window::focused() const noexcept {
    return impl_ != nullptr && impl_->window != nullptr &&
           (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

bool Window::set_vsync(vsync_mode mode) noexcept {
    return backend() == graphics_backend::opengl && impl_ != nullptr && impl_->context != nullptr &&
           SDL_GL_SetSwapInterval(static_cast<int>(mode));
}

void Window::set_resizable(bool enabled) {
    require_sdl(SDL_SetWindowResizable(impl_->window, enabled), error_code::window_creation,
                "Failed to change resizable state");
}

void Window::set_mouse_grabbed(bool enabled) {
    require_sdl(SDL_SetWindowMouseGrab(impl_->window, enabled), error_code::invalid_operation,
                "Failed to change mouse grab");
}

void Window::set_relative_mouse_mode(bool enabled) {
    require_sdl(SDL_SetWindowRelativeMouseMode(impl_->window, enabled), error_code::invalid_operation,
                "Failed to change relative mouse mode");
}

void Window::set_cursor_visible(bool visible) {
    require_sdl(visible ? SDL_ShowCursor() : SDL_HideCursor(), error_code::invalid_operation,
                "Failed to change cursor visibility");
}

void Window::start_text_input() {
    require_sdl(SDL_StartTextInput(impl_->window), error_code::invalid_operation,
                "Failed to start text input");
}

void Window::stop_text_input() {
    require_sdl(SDL_StopTextInput(impl_->window), error_code::invalid_operation,
                "Failed to stop text input");
}

void Window::set_clipboard_text(const std::string& text) {
    require_sdl(SDL_SetClipboardText(text.c_str()), error_code::invalid_operation,
                "Failed to set clipboard text");
}

std::string Window::clipboard_text() {
    char* value = SDL_GetClipboardText();
    if (value == nullptr) {
        throw_sdl(error_code::invalid_operation, "Failed to read clipboard text");
    }
    std::string result(value);
    SDL_free(value);
    return result;
}

bool Window::has_clipboard_text() noexcept {
    return SDL_HasClipboardText();
}

void* Window::native_window_handle() const noexcept {
    return impl_ != nullptr ? static_cast<void*>(impl_->window) : nullptr;
}

void* Window::native_gl_context() const noexcept {
    return impl_ != nullptr ? static_cast<void*>(impl_->context) : nullptr;
}

std::vector<std::string> Window::vulkan_instance_extensions() const {
    if (backend() != graphics_backend::vulkan) {
        throw error(error_code::invalid_operation,
                    "Vulkan extensions require a Vulkan window.");
    }

    std::uint32_t count = 0;
    const char* const* names = SDL_Vulkan_GetInstanceExtensions(&count);
    if (names == nullptr) {
        throw_sdl(error_code::graphics_initialization,
                  "Failed to query Vulkan instance extensions");
    }

    std::vector<std::string> result;
    result.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        result.emplace_back(names[index]);
    }
    return result;
}

std::uintptr_t Window::create_vulkan_surface(
    std::uintptr_t instance,
    const void* allocation_callbacks) const {
    if (backend() != graphics_backend::vulkan || impl_ == nullptr || impl_->window == nullptr) {
        throw error(error_code::invalid_operation,
                    "A valid Vulkan window is required to create a surface.");
    }

    VkSurfaceKHR surface{};
    if (!SDL_Vulkan_CreateSurface(
            impl_->window,
            handle_from_uintptr<VkInstance>(instance),
            static_cast<const VkAllocationCallbacks*>(allocation_callbacks),
            &surface)) {
        throw_sdl(error_code::graphics_initialization,
                  "Failed to create Vulkan window surface");
    }
    return handle_to_uintptr(surface);
}

} // namespace coffee

coffee::native::opengl_proc_type coffee::native::opengl_proc_address(const char* name) noexcept {
    if (name == nullptr) return nullptr;
    return SDL_GL_GetProcAddress(name);
}
