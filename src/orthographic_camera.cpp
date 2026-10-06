#include <coffee/orthographic_camera.h>

namespace coffee {

OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top,
                                       float near_plane, float far_plane)
    : projection_(orthographic(left, right, bottom, top, near_plane, far_plane)) {
    recalculate();
}
void OrthographicCamera::set_projection(float left, float right, float bottom, float top,
                                        float near_plane, float far_plane) {
    projection_ = orthographic(left, right, bottom, top, near_plane, far_plane);
    recalculate();
}
void OrthographicCamera::set_position(vec3 position) noexcept { position_ = position; recalculate(); }
void OrthographicCamera::set_rotation(float radians_value) noexcept { rotation_ = radians_value; recalculate(); }
vec3 OrthographicCamera::position() const noexcept { return position_; }
float OrthographicCamera::rotation() const noexcept { return rotation_; }
const mat4& OrthographicCamera::projection() const noexcept { return projection_; }
const mat4& OrthographicCamera::view() const noexcept { return view_; }
const mat4& OrthographicCamera::view_projection() const noexcept { return view_projection_; }
void OrthographicCamera::recalculate() noexcept {
    // Inverse of T * R for a 2D camera: R(-angle) * T(-position).
    view_ = rotate_z(-rotation_) * translate(-position_);
    view_projection_ = projection_ * view_;
}

} // namespace coffee
