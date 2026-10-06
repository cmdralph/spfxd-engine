#include <coffee/coffee.h>

#include <cmath>
#include <exception>
#include <iostream>

int main() {
    try {
        coffee::Window window({
            .title = "Coffee sandbox",
            .width = 1280,
            .height = 720,
            .samples = 4
        });

        coffee::Renderer renderer;
        coffee::SpriteBatch sprites;
        coffee::frame_timer timer;

        renderer.set_depth_test(false);

        while (window.is_open()) {
            window.poll_events();
            const double delta = timer.tick();
            static_cast<void>(delta);

            if (window.input().key_pressed(coffee::key::escape)) {
                window.close();
            }

            const auto pixels = window.framebuffer_size();
            renderer.set_viewport(0, 0, pixels.width, pixels.height);
            renderer.clear({0.035f, 0.045f, 0.065f, 1.0f}, true, false);

            coffee::OrthographicCamera camera(
                0.0f,
                static_cast<float>(pixels.width),
                static_cast<float>(pixels.height),
                0.0f
            );

            const float time = static_cast<float>(timer.elapsed_seconds());
            const float pulse = 0.85f + std::sin(time * 2.0f) * 0.15f;

            sprites.begin(camera.view_projection());
            sprites.draw_rect(
                {static_cast<float>(pixels.width) * 0.5f, static_cast<float>(pixels.height) * 0.5f},
                {280.0f * pulse, 180.0f * pulse},
                {0.35f, 0.72f, 0.95f, 1.0f},
                time * 0.35f
            );
            sprites.draw_rect(
                {80.0f, 80.0f},
                {100.0f, 100.0f},
                {0.95f, 0.52f, 0.35f, 0.9f},
                -time * 0.6f,
                {0.0f, 0.0f}
            );
            sprites.end();

            window.present();
        }
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Coffee error: " << exception.what() << '\n';
        return 1;
    }
}

