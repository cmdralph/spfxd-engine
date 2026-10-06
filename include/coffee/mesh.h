#pragma once

#include <coffee/api.h>
#include <coffee/math.h>
#include <coffee/vertex_array.h>

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace coffee {

struct mesh_vertex {
    vec3 position{};
    vec3 normal{0.0f, 1.0f, 0.0f};
    vec2 texture_coordinate{};
};

struct COFFEE_API mesh_data {
    std::vector<mesh_vertex> vertices;
    std::vector<std::uint32_t> indices;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] static mesh_data cube(float size = 1.0f);
    [[nodiscard]] static mesh_data plane(float width = 1.0f, float depth = 1.0f);
};

class COFFEE_API Mesh final {
public:
    explicit Mesh(const mesh_data& data, buffer_usage usage = buffer_usage::static_draw);

    [[nodiscard]] const VertexArray& vertex_array() const noexcept;
    [[nodiscard]] std::size_t index_count() const noexcept;

private:
    std::shared_ptr<VertexBuffer> vertices_;
    std::shared_ptr<IndexBuffer> indices_;
    VertexArray vertex_array_;
};

} // namespace coffee
