#include <coffee/sprite_batch.h>

#include <coffee/buffer.h>
#include <coffee/error.h>
#include <coffee/renderer.h>
#include <coffee/shader.h>
#include <coffee/vertex_array.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace coffee {

namespace {

constexpr std::string_view sprite_vertex_source = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_view_projection;

out vec4 v_color;
out vec2 v_uv;

void main() {
    v_color = a_color;
    v_uv = a_uv;
    gl_Position = u_view_projection * vec4(a_position, 1.0);
}
)glsl";

constexpr std::string_view sprite_fragment_source = R"glsl(
#version 330 core
in vec4 v_color;
in vec2 v_uv;

uniform sampler2D u_texture;

out vec4 fragment_color;

void main() {
    fragment_color = texture(u_texture, v_uv) * v_color;
}
)glsl";

struct sprite_vertex {
    vec3 position;
    color tint;
    vec2 uv;
};

} // namespace

struct SpriteBatch::impl {
    explicit impl(std::size_t requested_quads)
        : maximum_quads(checked_capacity(requested_quads)),
          vertices(maximum_quads * 4),
          vertex_buffer(std::make_shared<VertexBuffer>(maximum_quads * 4 * sizeof(sprite_vertex))),
          index_buffer(create_indices(maximum_quads)),
          shader(sprite_vertex_source, sprite_fragment_source),
          white_texture(Texture2D::white()) {
        vertex_array.add_vertex_buffer(vertex_buffer, {
            {"position", shader_data_type::float_3},
            {"color", shader_data_type::float_4},
            {"uv", shader_data_type::float_2}
        });
        vertex_array.set_index_buffer(index_buffer);
        shader.set_int("u_texture", 0);
    }

    static std::size_t checked_capacity(std::size_t quads) {
        constexpr auto maximum = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) / 4;
        if (quads == 0 || quads > maximum || quads > std::numeric_limits<std::size_t>::max() / (4 * sizeof(sprite_vertex))) {
            throw error(error_code::invalid_argument, "Sprite batch capacity is outside the supported range.");
        }
        return quads;
    }

    static std::shared_ptr<IndexBuffer> create_indices(std::size_t quads) {
        std::vector<std::uint32_t> indices(quads * 6);
        std::uint32_t vertex_offset = 0;
        for (std::size_t index = 0; index < indices.size(); index += 6) {
            indices[index + 0] = vertex_offset + 0;
            indices[index + 1] = vertex_offset + 1;
            indices[index + 2] = vertex_offset + 2;
            indices[index + 3] = vertex_offset + 2;
            indices[index + 4] = vertex_offset + 3;
            indices[index + 5] = vertex_offset + 0;
            vertex_offset += 4;
        }
        return std::make_shared<IndexBuffer>(indices);
    }

    std::size_t maximum_quads = 0;
    std::size_t quad_count = 0;
    std::vector<sprite_vertex> vertices;
    std::shared_ptr<VertexBuffer> vertex_buffer;
    std::shared_ptr<IndexBuffer> index_buffer;
    VertexArray vertex_array;
    Shader shader;
    Renderer renderer;
    std::shared_ptr<Texture2D> white_texture;
    std::shared_ptr<Texture2D> active_texture;
    sprite_batch_stats statistics{};
    bool recording = false;

    void flush_current() {
        if (quad_count == 0) return;
        const auto used_vertices = std::span<const sprite_vertex>(vertices.data(), quad_count * 4);
        vertex_buffer->set_data(used_vertices);
        active_texture->bind(0);
        shader.bind();
        renderer.draw(vertex_array, primitive_type::triangles, quad_count * 6);
        ++statistics.draw_calls;
        quad_count = 0;
    }
};

SpriteBatch::SpriteBatch(std::size_t maximum_quads) : impl_(std::make_unique<impl>(maximum_quads)) {}
SpriteBatch::~SpriteBatch() = default;
SpriteBatch::SpriteBatch(SpriteBatch&& other) noexcept = default;
SpriteBatch& SpriteBatch::operator=(SpriteBatch&& other) noexcept = default;

void SpriteBatch::begin(const mat4& view_projection) {
    if (impl_->recording) throw error(error_code::invalid_operation, "SpriteBatch::begin called while already active.");
    impl_->recording = true;
    impl_->quad_count = 0;
    impl_->statistics = {};
    impl_->active_texture = impl_->white_texture;
    impl_->shader.set_mat4("u_view_projection", view_projection);
    impl_->renderer.set_blending(true);
    impl_->renderer.set_blend_function(blend_factor::source_alpha, blend_factor::one_minus_source_alpha);
}

void SpriteBatch::draw_rect(vec2 position, vec2 size, color tint,
                            float rotation_radians, vec2 origin) {
    draw_texture(impl_->white_texture, position, size, tint, {}, {1.0f, 1.0f}, rotation_radians, origin);
}

void SpriteBatch::draw_texture(const std::shared_ptr<Texture2D>& texture, vec2 position, vec2 size,
                               color tint, vec2 uv_min, vec2 uv_max,
                               float rotation_radians, vec2 origin) {
    if (!impl_->recording) throw error(error_code::invalid_operation, "Draw calls require SpriteBatch::begin first.");
    if (!texture) throw error(error_code::invalid_argument, "Sprite texture cannot be null.");
    if (size.x == 0.0f || size.y == 0.0f) return;

    if (impl_->quad_count == impl_->maximum_quads ||
        impl_->active_texture->native_handle() != texture->native_handle()) {
        impl_->flush_current();
        impl_->active_texture = texture;
    }

    const float cosine = std::cos(rotation_radians);
    const float sine = std::sin(rotation_radians);
    const std::array<vec2, 4> local{
        vec2{-origin.x * size.x, -origin.y * size.y},
        vec2{(1.0f - origin.x) * size.x, -origin.y * size.y},
        vec2{(1.0f - origin.x) * size.x, (1.0f - origin.y) * size.y},
        vec2{-origin.x * size.x, (1.0f - origin.y) * size.y}
    };
    const std::array<vec2, 4> uvs{
        vec2{uv_min.x, uv_min.y}, vec2{uv_max.x, uv_min.y},
        vec2{uv_max.x, uv_max.y}, vec2{uv_min.x, uv_max.y}
    };

    const std::size_t offset = impl_->quad_count * 4;
    for (std::size_t index = 0; index < 4; ++index) {
        const vec2 rotated{
            local[index].x * cosine - local[index].y * sine,
            local[index].x * sine + local[index].y * cosine
        };
        impl_->vertices[offset + index] = {
            vec3{position + rotated, 0.0f}, tint, uvs[index]
        };
    }
    ++impl_->quad_count;
    ++impl_->statistics.quad_count;
}

void SpriteBatch::end() {
    if (!impl_->recording) throw error(error_code::invalid_operation, "SpriteBatch::end called while inactive.");
    impl_->flush_current();
    impl_->recording = false;
}

void SpriteBatch::flush() {
    if (!impl_->recording) throw error(error_code::invalid_operation, "SpriteBatch::flush called while inactive.");
    impl_->flush_current();
}

bool SpriteBatch::active() const noexcept { return impl_ != nullptr && impl_->recording; }
sprite_batch_stats SpriteBatch::stats() const noexcept { return impl_ != nullptr ? impl_->statistics : sprite_batch_stats{}; }

} // namespace coffee
