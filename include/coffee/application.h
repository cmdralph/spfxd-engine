#pragma once

#include <coffee/api.h>
#include <coffee/log.h>
#include <coffee/math.h>
#include <coffee/window.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace coffee {

class Renderer;
struct event;

struct application_config {
    window_config window{};
    color clear_color{0.035f, 0.045f, 0.065f, 1.0f};
    bool clear_each_frame = true;
    bool clear_depth = true;
    bool clear_stencil = false;
    bool auto_present = true;
    bool close_on_escape = false;
    bool pause_when_minimized = true;
    double maximum_delta_seconds = 0.25;
    double fixed_update_seconds = 0.0;
    std::size_t maximum_fixed_updates_per_frame = 8;
};

// A small, overridable application loop. It owns one Window and one Renderer
// but leaves scene organization, resources, and rendering policy to the user.
class COFFEE_API Application {
public:
    explicit Application(application_config config = {});
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    int run();
    void request_exit(int exit_code = 0) noexcept;

    [[nodiscard]] Window& window() noexcept;
    [[nodiscard]] const Window& window() const noexcept;
    [[nodiscard]] Renderer& renderer() noexcept;
    [[nodiscard]] const Renderer& renderer() const noexcept;

    [[nodiscard]] double delta_seconds() const noexcept;
    [[nodiscard]] double elapsed_seconds() const noexcept;
    [[nodiscard]] double frames_per_second() const noexcept;
    [[nodiscard]] std::uint64_t frame_index() const noexcept;
    [[nodiscard]] bool running() const noexcept;

protected:
    virtual void on_start();
    virtual void on_event(const event& value);
    virtual void on_fixed_update(double fixed_delta_seconds);
    virtual void on_update(double delta_seconds);
    virtual void on_render(double interpolation_alpha);
    virtual void on_shutdown() noexcept;

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

template<typename Type, typename... Arguments>
concept coffee_application =
    std::derived_from<Type, Application> &&
    std::constructible_from<Type, Arguments...>;

// A compact main() helper that reports uncaught startup/runtime failures
// through Coffee's logger and returns a conventional process exit code.
template<typename Type, typename... Arguments>
requires coffee_application<Type, Arguments...>
int run_application(Arguments&&... arguments) noexcept {
    try {
        Type application(std::forward<Arguments>(arguments)...);
        return application.run();
    } catch (const std::exception& exception) {
        log_critical("application", exception.what());
    } catch (...) {
        log_critical("application", "Unknown fatal exception.");
    }
    return 1;
}

} // namespace coffee
