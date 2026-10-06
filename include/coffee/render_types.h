#pragma once

#include <cstddef>
#include <cstdint>

namespace coffee {

enum class primitive_type { points, lines, line_strip, triangles, triangle_strip };
enum class buffer_usage { static_draw, dynamic_draw, stream_draw };
enum class blend_factor { zero, one, source_alpha, one_minus_source_alpha, destination_alpha, one_minus_destination_alpha };
enum class compare_function { never, less, equal, less_equal, greater, not_equal, greater_equal, always };
enum class cull_face { front, back, front_and_back };
enum class front_face { clockwise, counter_clockwise };

enum class shader_data_type {
    float_1,
    float_2,
    float_3,
    float_4,
    int_1,
    int_2,
    int_3,
    int_4,
    unsigned_byte_4
};

[[nodiscard]] constexpr std::size_t shader_data_size(shader_data_type type) noexcept {
    switch (type) {
        case shader_data_type::float_1: return sizeof(float);
        case shader_data_type::float_2: return sizeof(float) * 2;
        case shader_data_type::float_3: return sizeof(float) * 3;
        case shader_data_type::float_4: return sizeof(float) * 4;
        case shader_data_type::int_1: return sizeof(std::int32_t);
        case shader_data_type::int_2: return sizeof(std::int32_t) * 2;
        case shader_data_type::int_3: return sizeof(std::int32_t) * 3;
        case shader_data_type::int_4: return sizeof(std::int32_t) * 4;
        case shader_data_type::unsigned_byte_4: return sizeof(std::uint8_t) * 4;
    }
    return 0;
}

[[nodiscard]] constexpr int shader_data_components(shader_data_type type) noexcept {
    switch (type) {
        case shader_data_type::float_1:
        case shader_data_type::int_1: return 1;
        case shader_data_type::float_2:
        case shader_data_type::int_2: return 2;
        case shader_data_type::float_3:
        case shader_data_type::int_3: return 3;
        case shader_data_type::float_4:
        case shader_data_type::int_4:
        case shader_data_type::unsigned_byte_4: return 4;
    }
    return 0;
}

} // namespace coffee

