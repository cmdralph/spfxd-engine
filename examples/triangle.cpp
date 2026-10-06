#include <coffee/coffee.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <span>

namespace {

struct vertex {
    coffee::vec3 position;
    coffee::color color;
};

constexpr std::string_view vertex_shader = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;

uniform mat4 u_transform;
out vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = u_transform * vec4(a_position, 1.0);
}
)glsl";

constexpr std::string_view fragment_shader = R"glsl(
#version 330 core
in vec4 v_color;
out vec4 fragment_color;

void main() {
    fragment_color = v_color;
}
)glsl";

} // namespace

int main() {
    try {
        coffee::Window window("Coffee triangle", 960, 540);
        coffee::Renderer renderer;
        coffee::Shader shader(vertex_shader, fragment_shader);
        coffee::frame_timer timer;

        constexpr std::array vertices{
            vertex{{ 0.0f,  0.7f, 0.0f}, {0.95f, 0.35f, 0.30f, 1.0f}},
            vertex{{-0.7f, -0.6f, 0.0f}, {0.30f, 0.85f, 0.50f, 1.0f}},
            vertex{{ 0.7f, -0.6f, 0.0f}, {0.30f, 0.55f, 0.95f, 1.0f}}
        };
        constexpr std::array<std::uint32_t, 3> indices{0, 1, 2};

        auto vertex_buffer = std::make_shared<coffee::VertexBuffer>(
            std::as_bytes(std::span(vertices))
        );
        auto index_buffer = std::make_shared<coffee::IndexBuffer>(indices);

        coffee::VertexArray vertex_array;
        vertex_array.add_vertex_buffer(vertex_buffer, {
            {"position", coffee::shader_data_type::float_3},
            {"color", coffee::shader_data_type::float_4}
        });
        vertex_array.set_index_buffer(index_buffer);

        while (window.is_open()) {
            window.poll_events();
            timer.tick();
            if (window.input().key_pressed(coffee::key::escape)) window.close();

            const auto pixels = window.framebuffer_size();
            renderer.set_viewport(0, 0, pixels.width, pixels.height);
            renderer.clear({0.04f, 0.05f, 0.075f, 1.0f});

            shader.set_mat4(
                "u_transform",
                coffee::rotate_z(static_cast<float>(timer.elapsed_seconds()) * 0.6f)
            );
            shader.bind();
            renderer.draw(vertex_array);
            window.present();
        }
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Coffee error: " << exception.what() << '\n';
        return 1;
    }
}

