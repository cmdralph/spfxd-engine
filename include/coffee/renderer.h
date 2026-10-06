#pragma once

#include <coffee/api.h>
#include <coffee/math.h>
#include <coffee/render_types.h>
#include <coffee/vertex_array.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace coffee {

struct render_statistics {
    std::uint64_t draw_calls = 0;
    std::uint64_t vertices = 0;
    std::uint64_t indices = 0;
    std::uint64_t instances = 0;
};

struct graphics_info {
    std::string vendor;
    std::string renderer;
    std::string version;
    std::string shading_language_version;
    int maximum_texture_units = 0;
    int maximum_texture_size = 0;
    int maximum_vertex_attributes = 0;
};

class COFFEE_API Renderer final {
public:
    Renderer() = default;

    // Resets per-frame counters. Application calls this automatically.
    void begin_frame() noexcept;
    [[nodiscard]] render_statistics statistics() const noexcept;
    [[nodiscard]] graphics_info information() const;

    void set_viewport(int x, int y, int width, int height) const;
    void clear(color value = {0.08f, 0.09f, 0.12f, 1.0f},
               bool color_buffer = true,
               bool depth_buffer = true,
               bool stencil_buffer = false) const noexcept;

    void set_blending(bool enabled) const noexcept;
    void set_blend_function(blend_factor source, blend_factor destination) const noexcept;
    void set_depth_test(bool enabled) const noexcept;
    void set_depth_function(compare_function function) const noexcept;
    void set_culling(bool enabled, cull_face face = cull_face::back,
                     front_face winding = front_face::counter_clockwise) const noexcept;
    void set_wireframe(bool enabled) const noexcept;

    void draw(const VertexArray& vertex_array,
              primitive_type primitive = primitive_type::triangles,
              std::size_t index_count = 0) const;
    void draw_arrays(const VertexArray& vertex_array,
                     std::size_t vertex_count,
                     primitive_type primitive = primitive_type::triangles,
                     std::size_t first_vertex = 0) const;
    void draw_instanced(const VertexArray& vertex_array,
                        std::size_t instance_count,
                        primitive_type primitive = primitive_type::triangles,
                        std::size_t index_count = 0) const;

private:
    mutable render_statistics statistics_{};
};

} // namespace coffee
