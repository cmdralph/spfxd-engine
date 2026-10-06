#include <coffee/perspective_camera.h>
#include <coffee/error.h>

namespace coffee {

PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float near_plane, float far_plane) {
    set_perspective(fov, aspect, near_plane, far_plane);
}

void PerspectiveCamera::set_perspective(float fov, float aspect, float near_plane, float far_plane) {
    if (fov <= 0.0f || fov >= pi || aspect <= 0.0f || near_plane <= 0.0f || far_plane <= near_plane) {
        throw error(error_code::invalid_argument, "Invalid perspective camera parameters.");
    }
    projection_ = perspective(fov, aspect, near_plane, far_plane);
    recalculate();
}

void PerspectiveCamera::look_at(vec3 position_value, vec3 target, vec3 up) {
    if (length(target - position_value) == 0.0f || length(cross(target - position_value, up)) == 0.0f) {
        throw error(error_code::invalid_argument, "Camera direction and up vector must define a valid basis.");
    }
    position_ = position_value;
    view_ = coffee::look_at(position_value, target, up);
    recalculate();
}

vec3 PerspectiveCamera::position() const noexcept { return position_; }
const mat4& PerspectiveCamera::projection() const noexcept { return projection_; }
const mat4& PerspectiveCamera::view() const noexcept { return view_; }
const mat4& PerspectiveCamera::view_projection() const noexcept { return view_projection_; }
void PerspectiveCamera::recalculate() noexcept { view_projection_ = projection_ * view_; }

} // namespace coffee
