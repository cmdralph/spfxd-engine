#include <coffee/application.h>

#include <coffee/clock.h>
#include <coffee/error.h>
#include <coffee/event.h>
#include <coffee/input.h>
#include <coffee/renderer.h>

#include <algorithm>
#include <chrono>
#include <optional>
#include <thread>

namespace coffee {

struct Application::impl {
    explicit impl(application_config value)
        : config(std::move(value)),
          application_window(config.window),
          timer(config.maximum_delta_seconds > 0.0
                    ? config.maximum_delta_seconds
                    : 0.25) {
        if (config.fixed_update_seconds > 0.0) {
            fixed_step.emplace(config.fixed_update_seconds,
                               config.maximum_fixed_updates_per_frame);
        }
    }

    application_config config;
    Window application_window;
    Renderer application_renderer;
    frame_timer timer;
    std::optional<fixed_stepper> fixed_step;
    std::uint64_t frame = 0;
    int exit_code = 0;
    bool exit_requested = false;
    bool inside_run = false;
    bool has_run = false;
};

Application::Application(application_config config)
    : impl_(std::make_unique<impl>(std::move(config))) {}

Application::~Application() = default;

int Application::run() {
    if (impl_->inside_run || impl_->has_run) {
        throw error(error_code::invalid_operation,
                    "Application::run() may be called exactly once.");
    }
    impl_->has_run = true;
    impl_->inside_run = true;

    bool started = false;
    try {
        on_start();
        started = true;

        while (impl_->application_window.is_open() && !impl_->exit_requested) {
            impl_->application_window.poll_events();
            impl_->timer.tick();

            for (const auto& value : impl_->application_window.events()) {
                on_event(value);
            }

            if (impl_->config.close_on_escape &&
                impl_->application_window.input().key_pressed(key::escape)) {
                request_exit();
            }
            if (impl_->exit_requested || !impl_->application_window.is_open()) {
                break;
            }

            const auto framebuffer = impl_->application_window.framebuffer_size();
            if (impl_->config.pause_when_minimized &&
                (impl_->application_window.minimized() ||
                 framebuffer.width <= 0 || framebuffer.height <= 0)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
                continue;
            }

            double interpolation = 1.0;
            if (impl_->fixed_step) {
                impl_->fixed_step->advance(impl_->timer.delta_seconds(),
                    [this](double step) { on_fixed_update(step); });
                interpolation = impl_->fixed_step->interpolation_alpha();
            }

            on_update(impl_->timer.delta_seconds());
            impl_->application_renderer.begin_frame();

            if (impl_->config.clear_each_frame &&
                impl_->application_window.backend() == graphics_backend::opengl) {
                impl_->application_renderer.clear(
                    impl_->config.clear_color,
                    true,
                    impl_->config.clear_depth,
                    impl_->config.clear_stencil);
            }

            on_render(interpolation);

            if (impl_->config.auto_present &&
                impl_->application_window.backend() == graphics_backend::opengl) {
                impl_->application_window.present();
            }
            ++impl_->frame;
        }
    } catch (...) {
        if (started) {
            on_shutdown();
        }
        impl_->inside_run = false;
        throw;
    }

    if (started) {
        on_shutdown();
    }
    impl_->inside_run = false;
    return impl_->exit_code;
}

void Application::request_exit(int exit_code) noexcept {
    impl_->exit_code = exit_code;
    impl_->exit_requested = true;
    impl_->application_window.close();
}

Window& Application::window() noexcept { return impl_->application_window; }
const Window& Application::window() const noexcept { return impl_->application_window; }
Renderer& Application::renderer() noexcept { return impl_->application_renderer; }
const Renderer& Application::renderer() const noexcept { return impl_->application_renderer; }
double Application::delta_seconds() const noexcept { return impl_->timer.delta_seconds(); }
double Application::elapsed_seconds() const noexcept { return impl_->timer.elapsed_seconds(); }
double Application::frames_per_second() const noexcept { return impl_->timer.frames_per_second(); }
std::uint64_t Application::frame_index() const noexcept { return impl_->frame; }
bool Application::running() const noexcept {
    return impl_->inside_run && impl_->application_window.is_open() && !impl_->exit_requested;
}

void Application::on_start() {}
void Application::on_event(const event&) {}
void Application::on_fixed_update(double) {}
void Application::on_update(double) {}
void Application::on_render(double) {}
void Application::on_shutdown() noexcept {}

} // namespace coffee
