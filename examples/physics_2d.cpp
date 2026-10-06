#include <coffee/coffee.h>

#include <exception>
#include <iostream>
#include <vector>

int main() {
    try {
        coffee::Window window("Coffee 2D physics", 1000, 700);
        coffee::Renderer renderer;
        coffee::SpriteBatch sprites;
        coffee::PhysicsWorld2D physics;
        coffee::frame_timer timer;
        coffee::fixed_stepper fixed(1.0 / 120.0);

        const auto floor = physics.create_body({
            .type = coffee::body_type_2d::static_body,
            .shape = coffee::box_shape_2d{{8.0f, 0.5f}},
            .position = {0.0f, -4.5f}
        });
        static_cast<void>(floor);
        std::vector<coffee::body_handle_2d> boxes;
        for (int index = 0; index < 12; ++index) {
            boxes.push_back(physics.create_body({
                .shape = coffee::box_shape_2d{{0.45f, 0.45f}},
                .position = {-2.0f + static_cast<float>(index % 4) * 1.1f,
                             -2.8f + static_cast<float>(index / 4) * 1.1f},
                .mass = 1.0f,
                .restitution = 0.15f,
                .friction = 0.7f
            }));
        }

        renderer.set_depth_test(false);
        while (window.is_open()) {
            window.poll_events();
            const double delta = timer.tick();
            if (window.input().key_pressed(coffee::key::escape)) window.close();
            fixed.advance(delta, [&](double step) { physics.step(static_cast<float>(step)); });

            const auto pixels = window.framebuffer_size();
            renderer.set_viewport(0, 0, pixels.width, pixels.height);
            renderer.clear({0.035f, 0.045f, 0.065f, 1.0f}, true, false);
            coffee::OrthographicCamera camera(-8.0f, 8.0f, -5.6f, 5.6f);
            sprites.begin(camera.view_projection());
            sprites.draw_rect({0.0f, -4.5f}, {16.0f, 1.0f}, {0.22f, 0.28f, 0.36f, 1.0f});
            for (const auto body : boxes) {
                sprites.draw_rect(physics.position(body), {0.9f, 0.9f}, {0.30f, 0.72f, 0.95f, 1.0f});
            }
            sprites.end();
            window.present();
        }
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Coffee error: " << exception.what() << '\n';
        return 1;
    }
}
