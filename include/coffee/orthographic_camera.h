#pragma once

#include <coffee/api.h>
#include <coffee/math.h>

namespace coffee {

class COFFEE_API OrthographicCamera final {
public:
    OrthographicCamera(float left, float right, float bottom, float top,
                       float near_plane = -1.0f, float far_plane = 1.0f);

    void set_projection(float left, float right, float bottom, float top,
                        float near_plane = -1.0f, float far_plane = 1.0f);
    void set_position(vec3 position) noexcept;
    void set_rotation(float radians_value) noexcept;

    [[nodiscard]] vec3 position() const noexcept;
    [[nodiscard]] float rotation() const noexcept;
    [[nodiscard]] const mat4& projection() const noexcept;
    [[nodiscard]] const mat4& view() const noexcept;
    [[nodiscard]] const mat4& view_projection() const noexcept;

private:
    void recalculate() noexcept;
    mat4 projection_ = mat4::identity();
    mat4 view_ = mat4::identity();
    mat4 view_projection_ = mat4::identity();
    vec3 position_{};
    float rotation_ = 0.0f;
};

} // namespace coffee
