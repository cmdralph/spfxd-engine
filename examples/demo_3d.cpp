#include <coffee/coffee.h>

#include <exception>
#include <iostream>

namespace {
constexpr std::string_view vertex_source = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
uniform mat4 u_mvp;
uniform mat4 u_model;
out vec3 v_normal;
void main() {
    v_normal = mat3(u_model) * a_normal;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)glsl";
constexpr std::string_view fragment_source = R"glsl(
#version 330 core
in vec3 v_normal;
out vec4 fragment_color;
void main() {
    vec3 normal = normalize(v_normal);
    float light = max(dot(normal, normalize(vec3(0.4, 0.8, 0.5))), 0.0);
    vec3 base = vec3(0.24, 0.66, 0.95);
    fragment_color = vec4(base * (0.22 + light * 0.78), 1.0);
}
)glsl";
}

int main() {
    try {
        coffee::Window window("Coffee 3D", 1100, 700);
        coffee::Renderer renderer;
        coffee::Shader shader(vertex_source, fragment_source);
        coffee::Mesh cube(coffee::mesh_data::cube());
        coffee::frame_timer timer;
        renderer.set_depth_test(true);
        renderer.set_culling(true);

        while (window.is_open()) {
            window.poll_events();
            timer.tick();
            if (window.input().key_pressed(coffee::key::escape)) window.close();
            const auto pixels = window.framebuffer_size();
            if (pixels.height == 0) continue;
            renderer.set_viewport(0, 0, pixels.width, pixels.height);
            renderer.clear({0.025f, 0.035f, 0.055f, 1.0f});
            coffee::PerspectiveCamera camera(
                coffee::radians(60.0f),
                static_cast<float>(pixels.width) / static_cast<float>(pixels.height));
            camera.look_at({2.8f, 2.1f, 3.4f}, {0.0f, 0.0f, 0.0f});
            const float time = static_cast<float>(timer.elapsed_seconds());
            const coffee::mat4 model = coffee::rotate_y(time * 0.7f) * coffee::rotate_x(time * 0.35f);
            shader.set_mat4("u_model", model);
            shader.set_mat4("u_mvp", camera.view_projection() * model);
            renderer.draw(cube.vertex_array());
            window.present();
        }
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Coffee error: " << exception.what() << '\n';
        return 1;
    }
}

