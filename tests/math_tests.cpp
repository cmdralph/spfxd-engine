#include <coffee/math.h>

#include <cmath>
#include <iostream>

namespace {

bool nearly_equal(float left, float right, float epsilon = 0.0001f) {
    return std::abs(left - right) <= epsilon;
}

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

} // namespace

int main() {
    using namespace coffee;

    expect(nearly_equal(length(vec2{3.0f, 4.0f}), 5.0f), "vec2 length");
    expect(cross(vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}) == vec3{0.0f, 0.0f, 1.0f},
           "vec3 cross product");

    const vec4 translated = translate({2.0f, 3.0f, 4.0f}) * vec4{1.0f, 1.0f, 1.0f, 1.0f};
    expect(translated == vec4{3.0f, 4.0f, 5.0f, 1.0f}, "translation matrix");

    const vec4 scaled = scale({2.0f, 3.0f, 4.0f}) * vec4{1.0f, 1.0f, 1.0f, 1.0f};
    expect(scaled == vec4{2.0f, 3.0f, 4.0f, 1.0f}, "scale matrix");

    const vec4 rotated = rotate_z(radians(90.0f)) * vec4{1.0f, 0.0f, 0.0f, 1.0f};
    expect(nearly_equal(rotated.x, 0.0f) && nearly_equal(rotated.y, 1.0f), "rotation matrix");

    const vec4 center = orthographic(-10.0f, 10.0f, -5.0f, 5.0f) * vec4{0.0f, 0.0f, 0.0f, 1.0f};
    expect(nearly_equal(center.x, 0.0f) && nearly_equal(center.y, 0.0f), "orthographic center");

    const auto quarter_turn = quaternion::from_axis_angle({0.0f, 1.0f, 0.0f}, radians(90.0f));
    const vec3 turned = transform_point(rotation(quarter_turn), {1.0f, 0.0f, 0.0f});
    expect(nearly_equal(turned.x, 0.0f) && nearly_equal(turned.z, -1.0f), "quaternion rotation");

    const vec3 camera_origin = transform_point(look_at({0.0f, 0.0f, 5.0f}, {}), {0.0f, 0.0f, 5.0f});
    expect(nearly_equal(camera_origin.x, 0.0f) && nearly_equal(camera_origin.y, 0.0f) &&
           nearly_equal(camera_origin.z, 0.0f), "look-at view matrix");

    if (failures == 0) {
        std::cout << "All Coffee math tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
