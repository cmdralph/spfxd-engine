#pragma once

#include <coffee/api.h>
#include <coffee/math.h>

namespace coffee {

class COFFEE_API PerspectiveCamera final {
public:
    PerspectiveCamera(float vertical_fov_radians, float aspect_ratio,
                      float near_plane = 0.1f, float far_plane = 1000.0f);

    void set_perspective(float vertical_fov_radians, float aspect_ratio,
                         float near_plane, float far_plane);
    void look_at(vec3 position, vec3 target, vec3 up = {0.0f, 1.0f, 0.0f});

    [[nodiscard]] vec3 position() const noexcept;
    [[nodiscard]] const mat4& projection() const noexcept;
    [[nodiscard]] const mat4& view() const noexcept;
    [[nodiscard]] const mat4& view_projection() const noexcept;

private:
    void recalculate() noexcept;
    mat4 projection_ = mat4::identity();
    mat4 view_ = mat4::identity();
    mat4 view_projection_ = mat4::identity();
    vec3 position_{};
};

} // namespace coffee
