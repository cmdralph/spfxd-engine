#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace coffee {

inline constexpr float pi = std::numbers::pi_v<float>;

[[nodiscard]] constexpr float radians(float degrees_value) noexcept {
    return degrees_value * (pi / 180.0f);
}

[[nodiscard]] constexpr float degrees(float radians_value) noexcept {
    return radians_value * (180.0f / pi);
}

template<typename T>
[[nodiscard]] constexpr T clamp(T value, T minimum, T maximum) noexcept {
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

struct vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr vec2() = default;
    constexpr explicit vec2(float value) : x(value), y(value) {}
    constexpr vec2(float x_value, float y_value) : x(x_value), y(y_value) {}

    [[nodiscard]] constexpr vec2 operator-() const noexcept { return {-x, -y}; }
    [[nodiscard]] constexpr vec2 operator+(vec2 rhs) const noexcept { return {x + rhs.x, y + rhs.y}; }
    [[nodiscard]] constexpr vec2 operator-(vec2 rhs) const noexcept { return {x - rhs.x, y - rhs.y}; }
    [[nodiscard]] constexpr vec2 operator*(float scalar) const noexcept { return {x * scalar, y * scalar}; }
    [[nodiscard]] constexpr vec2 operator/(float scalar) const noexcept { return {x / scalar, y / scalar}; }
    constexpr vec2& operator+=(vec2 rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
    constexpr vec2& operator-=(vec2 rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }
    constexpr vec2& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; return *this; }
    [[nodiscard]] constexpr bool operator==(const vec2&) const noexcept = default;
};

[[nodiscard]] constexpr vec2 operator*(float scalar, vec2 value) noexcept { return value * scalar; }
[[nodiscard]] constexpr float dot(vec2 a, vec2 b) noexcept { return a.x * b.x + a.y * b.y; }
[[nodiscard]] inline float length(vec2 value) noexcept { return std::sqrt(dot(value, value)); }
[[nodiscard]] inline vec2 normalized(vec2 value) noexcept {
    const float magnitude = length(value);
    return magnitude > 0.0f ? value / magnitude : vec2{};
}

struct vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr vec3() = default;
    constexpr explicit vec3(float value) : x(value), y(value), z(value) {}
    constexpr vec3(float x_value, float y_value, float z_value) : x(x_value), y(y_value), z(z_value) {}
    constexpr vec3(vec2 xy, float z_value = 0.0f) : x(xy.x), y(xy.y), z(z_value) {}

    [[nodiscard]] constexpr vec3 operator-() const noexcept { return {-x, -y, -z}; }
    [[nodiscard]] constexpr vec3 operator+(vec3 rhs) const noexcept { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    [[nodiscard]] constexpr vec3 operator-(vec3 rhs) const noexcept { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    [[nodiscard]] constexpr vec3 operator*(float scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }
    [[nodiscard]] constexpr vec3 operator/(float scalar) const noexcept { return {x / scalar, y / scalar, z / scalar}; }
    constexpr vec3& operator+=(vec3 rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    constexpr vec3& operator-=(vec3 rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    constexpr vec3& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
    [[nodiscard]] constexpr bool operator==(const vec3&) const noexcept = default;
};

[[nodiscard]] constexpr vec3 operator*(float scalar, vec3 value) noexcept { return value * scalar; }
[[nodiscard]] constexpr float dot(vec3 a, vec3 b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
[[nodiscard]] constexpr vec3 cross(vec3 a, vec3 b) noexcept {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
[[nodiscard]] inline float length(vec3 value) noexcept { return std::sqrt(dot(value, value)); }
[[nodiscard]] inline vec3 normalized(vec3 value) noexcept {
    const float magnitude = length(value);
    return magnitude > 0.0f ? value / magnitude : vec3{};
}

struct vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    constexpr vec4() = default;
    constexpr explicit vec4(float value) : x(value), y(value), z(value), w(value) {}
    constexpr vec4(float x_value, float y_value, float z_value, float w_value)
        : x(x_value), y(y_value), z(z_value), w(w_value) {}
    constexpr vec4(vec3 xyz, float w_value = 1.0f) : x(xyz.x), y(xyz.y), z(xyz.z), w(w_value) {}

    [[nodiscard]] constexpr vec4 operator+(vec4 rhs) const noexcept { return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w}; }
    [[nodiscard]] constexpr vec4 operator-(vec4 rhs) const noexcept { return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w}; }
    [[nodiscard]] constexpr vec4 operator*(float scalar) const noexcept { return {x * scalar, y * scalar, z * scalar, w * scalar}; }
    [[nodiscard]] constexpr vec4 operator/(float scalar) const noexcept { return {x / scalar, y / scalar, z / scalar, w / scalar}; }
    [[nodiscard]] constexpr bool operator==(const vec4&) const noexcept = default;
};

using color = vec4;

template<typename T>
[[nodiscard]] constexpr T lerp(const T& from, const T& to, float amount) noexcept {
    return from + (to - from) * amount;
}

[[nodiscard]] constexpr vec3 reflect(vec3 direction, vec3 normal) noexcept {
    return direction - 2.0f * dot(direction, normal) * normal;
}

struct mat4 {
    std::array<float, 16> values{};

    constexpr mat4() = default;
    constexpr explicit mat4(float diagonal) {
        values[0] = diagonal;
        values[5] = diagonal;
        values[10] = diagonal;
        values[15] = diagonal;
    }

    [[nodiscard]] static constexpr mat4 identity() noexcept { return mat4{1.0f}; }
    [[nodiscard]] constexpr float* data() noexcept { return values.data(); }
    [[nodiscard]] constexpr const float* data() const noexcept { return values.data(); }
    [[nodiscard]] constexpr float& operator()(std::size_t row, std::size_t column) noexcept {
        return values[column * 4 + row];
    }
    [[nodiscard]] constexpr float operator()(std::size_t row, std::size_t column) const noexcept {
        return values[column * 4 + row];
    }
    [[nodiscard]] constexpr bool operator==(const mat4&) const noexcept = default;
};

[[nodiscard]] constexpr mat4 operator*(const mat4& lhs, const mat4& rhs) noexcept {
    mat4 result{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t index = 0; index < 4; ++index) {
                result(row, column) += lhs(row, index) * rhs(index, column);
            }
        }
    }
    return result;
}

[[nodiscard]] constexpr vec4 operator*(const mat4& matrix, vec4 vector) noexcept {
    return {
        matrix(0, 0) * vector.x + matrix(0, 1) * vector.y + matrix(0, 2) * vector.z + matrix(0, 3) * vector.w,
        matrix(1, 0) * vector.x + matrix(1, 1) * vector.y + matrix(1, 2) * vector.z + matrix(1, 3) * vector.w,
        matrix(2, 0) * vector.x + matrix(2, 1) * vector.y + matrix(2, 2) * vector.z + matrix(2, 3) * vector.w,
        matrix(3, 0) * vector.x + matrix(3, 1) * vector.y + matrix(3, 2) * vector.z + matrix(3, 3) * vector.w
    };
}

[[nodiscard]] constexpr mat4 translate(vec3 offset) noexcept {
    mat4 result = mat4::identity();
    result(0, 3) = offset.x;
    result(1, 3) = offset.y;
    result(2, 3) = offset.z;
    return result;
}

[[nodiscard]] constexpr mat4 scale(vec3 amount) noexcept {
    mat4 result{};
    result(0, 0) = amount.x;
    result(1, 1) = amount.y;
    result(2, 2) = amount.z;
    result(3, 3) = 1.0f;
    return result;
}

[[nodiscard]] inline mat4 rotate_z(float angle_radians) noexcept {
    mat4 result = mat4::identity();
    const float cosine = std::cos(angle_radians);
    const float sine = std::sin(angle_radians);
    result(0, 0) = cosine;
    result(1, 0) = sine;
    result(0, 1) = -sine;
    result(1, 1) = cosine;
    return result;
}

[[nodiscard]] inline mat4 rotate_x(float angle_radians) noexcept {
    mat4 result = mat4::identity();
    const float cosine = std::cos(angle_radians);
    const float sine = std::sin(angle_radians);
    result(1, 1) = cosine;
    result(2, 1) = sine;
    result(1, 2) = -sine;
    result(2, 2) = cosine;
    return result;
}

[[nodiscard]] inline mat4 rotate_y(float angle_radians) noexcept {
    mat4 result = mat4::identity();
    const float cosine = std::cos(angle_radians);
    const float sine = std::sin(angle_radians);
    result(0, 0) = cosine;
    result(2, 0) = -sine;
    result(0, 2) = sine;
    result(2, 2) = cosine;
    return result;
}

struct quaternion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    constexpr quaternion() = default;
    constexpr quaternion(float x_value, float y_value, float z_value, float w_value)
        : x(x_value), y(y_value), z(z_value), w(w_value) {}

    [[nodiscard]] static quaternion from_axis_angle(vec3 axis, float angle_radians) noexcept {
        axis = normalized(axis);
        const float half = angle_radians * 0.5f;
        const float sine = std::sin(half);
        return {axis.x * sine, axis.y * sine, axis.z * sine, std::cos(half)};
    }
};

[[nodiscard]] constexpr quaternion operator*(quaternion lhs, quaternion rhs) noexcept {
    return {
        lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
        lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
        lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z
    };
}

[[nodiscard]] inline quaternion normalized(quaternion value) noexcept {
    const float magnitude = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w);
    return magnitude > 0.0f
        ? quaternion{value.x / magnitude, value.y / magnitude, value.z / magnitude, value.w / magnitude}
        : quaternion{};
}

[[nodiscard]] inline mat4 rotation(quaternion value) noexcept {
    value = normalized(value);
    const float xx = value.x * value.x;
    const float yy = value.y * value.y;
    const float zz = value.z * value.z;
    const float xy = value.x * value.y;
    const float xz = value.x * value.z;
    const float yz = value.y * value.z;
    const float wx = value.w * value.x;
    const float wy = value.w * value.y;
    const float wz = value.w * value.z;

    mat4 result = mat4::identity();
    result(0, 0) = 1.0f - 2.0f * (yy + zz);
    result(1, 0) = 2.0f * (xy + wz);
    result(2, 0) = 2.0f * (xz - wy);
    result(0, 1) = 2.0f * (xy - wz);
    result(1, 1) = 1.0f - 2.0f * (xx + zz);
    result(2, 1) = 2.0f * (yz + wx);
    result(0, 2) = 2.0f * (xz + wy);
    result(1, 2) = 2.0f * (yz - wx);
    result(2, 2) = 1.0f - 2.0f * (xx + yy);
    return result;
}

struct transform_3d {
    vec3 position{};
    quaternion orientation{};
    vec3 scale_value{1.0f};

    [[nodiscard]] mat4 matrix() const noexcept {
        return translate(position) * rotation(orientation) * scale(scale_value);
    }
};

[[nodiscard]] inline mat4 look_at(vec3 eye, vec3 target, vec3 up = {0.0f, 1.0f, 0.0f}) noexcept {
    const vec3 forward = normalized(target - eye);
    const vec3 side = normalized(cross(forward, up));
    const vec3 actual_up = cross(side, forward);
    mat4 result = mat4::identity();
    result(0, 0) = side.x; result(0, 1) = side.y; result(0, 2) = side.z;
    result(1, 0) = actual_up.x; result(1, 1) = actual_up.y; result(1, 2) = actual_up.z;
    result(2, 0) = -forward.x; result(2, 1) = -forward.y; result(2, 2) = -forward.z;
    result(0, 3) = -dot(side, eye);
    result(1, 3) = -dot(actual_up, eye);
    result(2, 3) = dot(forward, eye);
    return result;
}

[[nodiscard]] constexpr vec3 transform_point(const mat4& matrix, vec3 point) noexcept {
    const vec4 transformed = matrix * vec4{point, 1.0f};
    return transformed.w != 0.0f
        ? vec3{transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w}
        : vec3{transformed.x, transformed.y, transformed.z};
}

[[nodiscard]] constexpr mat4 orthographic(
    float left, float right, float bottom, float top,
    float near_plane = -1.0f, float far_plane = 1.0f) noexcept {
    mat4 result = mat4::identity();
    result(0, 0) = 2.0f / (right - left);
    result(1, 1) = 2.0f / (top - bottom);
    result(2, 2) = -2.0f / (far_plane - near_plane);
    result(0, 3) = -(right + left) / (right - left);
    result(1, 3) = -(top + bottom) / (top - bottom);
    result(2, 3) = -(far_plane + near_plane) / (far_plane - near_plane);
    return result;
}

[[nodiscard]] inline mat4 perspective(
    float vertical_fov_radians, float aspect_ratio,
    float near_plane, float far_plane) noexcept {
    const float tangent = std::tan(vertical_fov_radians * 0.5f);
    mat4 result{};
    result(0, 0) = 1.0f / (aspect_ratio * tangent);
    result(1, 1) = 1.0f / tangent;
    result(2, 2) = -(far_plane + near_plane) / (far_plane - near_plane);
    result(2, 3) = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);
    result(3, 2) = -1.0f;
    return result;
}

} // namespace coffee
