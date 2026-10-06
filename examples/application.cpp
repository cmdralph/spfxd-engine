#include <coffee/coffee.h>

#include <cmath>

namespace {

coffee::application_config make_config() {
    coffee::application_config config;
    config.window.title = "Coffee application";
    config.window.width = 1100;
    config.window.height = 700;
    config.window.samples = 4;
    config.clear_color = {0.025f, 0.035f, 0.055f, 1.0f};
    config.clear_depth = false;
    config.close_on_escape = true;
    return config;
}

class showcase final : public coffee::Application {
public:
    showcase() : Application(make_config()) {}

private:
    void on_start() override {
        renderer().set_depth_test(false);
        coffee::log_info("showcase", "Application started. Press Escape to close.");
    }

    void on_render(double) override {
        const auto pixels = window().framebuffer_size();
        if (pixels.width <= 0 || pixels.height <= 0) {
            return;
        }

        renderer().set_viewport(0, 0, pixels.width, pixels.height);
        coffee::OrthographicCamera camera(
            0.0f,
            static_cast<float>(pixels.width),
            static_cast<float>(pixels.height),
            0.0f);

        const float time = static_cast<float>(elapsed_seconds());
        const float pulse = 0.9f + std::sin(time * 2.0f) * 0.1f;
        sprites_.begin(camera.view_projection());
        sprites_.draw_rect(
            {static_cast<float>(pixels.width) * 0.5f,
             static_cast<float>(pixels.height) * 0.5f},
            {300.0f * pulse, 180.0f * pulse},
            {0.28f, 0.72f, 0.96f, 1.0f},
            time * 0.25f);
        sprites_.end();
    }

    void on_shutdown() noexcept override {
        coffee::log_info("showcase", "Application stopped cleanly.");
    }

    coffee::SpriteBatch sprites_;
};

} // namespace

int main() {
    return coffee::run_application<showcase>();
}
