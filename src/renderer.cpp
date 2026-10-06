#include <coffee/renderer.h>
#include <coffee/error.h>

#include <glad/gl.h>

#include <limits>
#include <string>

namespace coffee {

namespace {

GLenum to_gl(primitive_type value) noexcept {
    switch (value) {
        case primitive_type::points: return GL_POINTS;
        case primitive_type::lines: return GL_LINES;
        case primitive_type::line_strip: return GL_LINE_STRIP;
        case primitive_type::triangles: return GL_TRIANGLES;
        case primitive_type::triangle_strip: return GL_TRIANGLE_STRIP;
    }
    return GL_TRIANGLES;
}
GLenum to_gl(blend_factor value) noexcept {
    switch (value) {
        case blend_factor::zero: return GL_ZERO;
        case blend_factor::one: return GL_ONE;
        case blend_factor::source_alpha: return GL_SRC_ALPHA;
        case blend_factor::one_minus_source_alpha: return GL_ONE_MINUS_SRC_ALPHA;
        case blend_factor::destination_alpha: return GL_DST_ALPHA;
        case blend_factor::one_minus_destination_alpha: return GL_ONE_MINUS_DST_ALPHA;
    }
    return GL_ONE;
}
GLenum to_gl(compare_function value) noexcept {
    switch (value) {
        case compare_function::never: return GL_NEVER;
        case compare_function::less: return GL_LESS;
        case compare_function::equal: return GL_EQUAL;
        case compare_function::less_equal: return GL_LEQUAL;
        case compare_function::greater: return GL_GREATER;
        case compare_function::not_equal: return GL_NOTEQUAL;
        case compare_function::greater_equal: return GL_GEQUAL;
        case compare_function::always: return GL_ALWAYS;
    }
    return GL_LESS;
}
GLenum to_gl(cull_face value) noexcept {
    switch (value) {
        case cull_face::front: return GL_FRONT;
        case cull_face::back: return GL_BACK;
        case cull_face::front_and_back: return GL_FRONT_AND_BACK;
    }
    return GL_BACK;
}
GLsizei checked_count(std::size_t count) {
    if (count > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw error(error_code::invalid_argument, "Draw count is too large for OpenGL.");
    }
    return static_cast<GLsizei>(count);
}

GLint checked_first(std::size_t first) {
    if (first > static_cast<std::size_t>(std::numeric_limits<GLint>::max())) {
        throw error(error_code::invalid_argument, "First vertex is too large for OpenGL.");
    }
    return static_cast<GLint>(first);
}

[[nodiscard]] std::string gl_string(GLenum name) {
    const auto* value = glGetString(name);
    return value != nullptr
        ? reinterpret_cast<const char*>(value)
        : std::string{};
}

} // namespace

void Renderer::begin_frame() noexcept { statistics_ = {}; }
render_statistics Renderer::statistics() const noexcept { return statistics_; }

graphics_info Renderer::information() const {
    graphics_info result;
    result.vendor = gl_string(GL_VENDOR);
    result.renderer = gl_string(GL_RENDERER);
    result.version = gl_string(GL_VERSION);
    result.shading_language_version = gl_string(GL_SHADING_LANGUAGE_VERSION);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &result.maximum_texture_units);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &result.maximum_texture_size);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &result.maximum_vertex_attributes);
    return result;
}

void Renderer::set_viewport(int x, int y, int width, int height) const {
    if (width < 0 || height < 0) throw error(error_code::invalid_argument, "Viewport dimensions cannot be negative.");
    glViewport(x, y, width, height);
}
void Renderer::clear(color value, bool color_buffer, bool depth_buffer, bool stencil_buffer) const noexcept {
    glClearColor(value.x, value.y, value.z, value.w);
    GLbitfield mask = 0;
    if (color_buffer) mask |= GL_COLOR_BUFFER_BIT;
    if (depth_buffer) mask |= GL_DEPTH_BUFFER_BIT;
    if (stencil_buffer) mask |= GL_STENCIL_BUFFER_BIT;
    glClear(mask);
}
void Renderer::set_blending(bool enabled) const noexcept { enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND); }
void Renderer::set_blend_function(blend_factor source, blend_factor destination) const noexcept {
    glBlendFunc(to_gl(source), to_gl(destination));
}
void Renderer::set_depth_test(bool enabled) const noexcept { enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST); }
void Renderer::set_depth_function(compare_function function) const noexcept { glDepthFunc(to_gl(function)); }
void Renderer::set_culling(bool enabled, cull_face face, front_face winding) const noexcept {
    enabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glCullFace(to_gl(face));
    glFrontFace(winding == front_face::clockwise ? GL_CW : GL_CCW);
}
void Renderer::set_wireframe(bool enabled) const noexcept { glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL); }
void Renderer::draw(const VertexArray& vertex_array, primitive_type primitive, std::size_t index_count) const {
    const auto& indices = vertex_array.index_buffer();
    if (!indices) throw error(error_code::invalid_operation, "Indexed draw requires an index buffer.");
    const std::size_t count = index_count == 0 ? indices->count() : index_count;
    if (count > indices->count()) throw error(error_code::invalid_argument, "Draw count exceeds index buffer size.");
    vertex_array.bind();
    glDrawElements(to_gl(primitive), checked_count(count), GL_UNSIGNED_INT, nullptr);
    ++statistics_.draw_calls;
    statistics_.indices += count;
}
void Renderer::draw_arrays(const VertexArray& vertex_array, std::size_t vertex_count,
                           primitive_type primitive, std::size_t first_vertex) const {
    vertex_array.bind();
    glDrawArrays(to_gl(primitive), checked_first(first_vertex), checked_count(vertex_count));
    ++statistics_.draw_calls;
    statistics_.vertices += vertex_count;
}

void Renderer::draw_instanced(const VertexArray& vertex_array,
                              std::size_t instance_count,
                              primitive_type primitive,
                              std::size_t index_count) const {
    const auto& indices = vertex_array.index_buffer();
    if (!indices) {
        throw error(error_code::invalid_operation,
                    "Instanced indexed draw requires an index buffer.");
    }
    const std::size_t count = index_count == 0 ? indices->count() : index_count;
    if (count > indices->count()) {
        throw error(error_code::invalid_argument,
                    "Instanced draw count exceeds index buffer size.");
    }
    vertex_array.bind();
    glDrawElementsInstanced(to_gl(primitive), checked_count(count), GL_UNSIGNED_INT,
                            nullptr, checked_count(instance_count));
    ++statistics_.draw_calls;
    statistics_.indices += count * instance_count;
    statistics_.instances += instance_count;
}

} // namespace coffee
