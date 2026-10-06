#include <coffee/mesh.h>
#include <coffee/error.h>

#include <array>
#include <cstddef>
#include <span>

namespace coffee {

bool mesh_data::valid() const noexcept {
    if (vertices.empty() || indices.empty() || indices.size() % 3 != 0) return false;
    for (const auto index : indices) if (index >= vertices.size()) return false;
    return true;
}

mesh_data mesh_data::plane(float width, float depth) {
    if (width <= 0.0f || depth <= 0.0f) {
        throw error(error_code::invalid_argument, "Plane dimensions must be positive.");
    }
    const float x = width * 0.5f;
    const float z = depth * 0.5f;
    return {
        {
            {{-x, 0.0f, -z}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{ x, 0.0f, -z}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{ x, 0.0f,  z}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-x, 0.0f,  z}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
        },
        {0, 1, 2, 2, 3, 0}
    };
}

mesh_data mesh_data::cube(float size) {
    if (size <= 0.0f) throw error(error_code::invalid_argument, "Cube size must be positive.");
    const float h = size * 0.5f;
    mesh_data result;
    const auto add_face = [&result](vec3 a, vec3 b, vec3 c, vec3 d, vec3 normal) {
        const auto base = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(), {
            {a, normal, {0.0f, 0.0f}}, {b, normal, {1.0f, 0.0f}},
            {c, normal, {1.0f, 1.0f}}, {d, normal, {0.0f, 1.0f}}
        });
        result.indices.insert(result.indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});
    };
    add_face({-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h}, {0,0,1});
    add_face({ h,-h,-h}, {-h,-h,-h}, {-h, h,-h}, { h, h,-h}, {0,0,-1});
    add_face({-h,-h,-h}, {-h,-h, h}, {-h, h, h}, {-h, h,-h}, {-1,0,0});
    add_face({ h,-h, h}, { h,-h,-h}, { h, h,-h}, { h, h, h}, {1,0,0});
    add_face({-h, h, h}, { h, h, h}, { h, h,-h}, {-h, h,-h}, {0,1,0});
    add_face({-h,-h,-h}, { h,-h,-h}, { h,-h, h}, {-h,-h, h}, {0,-1,0});
    return result;
}

Mesh::Mesh(const mesh_data& data, buffer_usage usage) {
    if (!data.valid()) throw error(error_code::invalid_argument, "Mesh data is empty or contains invalid indices.");
    vertices_ = std::make_shared<VertexBuffer>(std::as_bytes(std::span(data.vertices)), usage);
    indices_ = std::make_shared<IndexBuffer>(data.indices, usage);
    vertex_array_.add_vertex_buffer(vertices_, {
        {"position", shader_data_type::float_3},
        {"normal", shader_data_type::float_3},
        {"texture_coordinate", shader_data_type::float_2}
    });
    vertex_array_.set_index_buffer(indices_);
}

const VertexArray& Mesh::vertex_array() const noexcept { return vertex_array_; }
std::size_t Mesh::index_count() const noexcept { return indices_ != nullptr ? indices_->count() : 0; }

} // namespace coffee
