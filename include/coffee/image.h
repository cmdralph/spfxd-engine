#pragma once

#include <coffee/api.h>

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace coffee {

enum class image_format { r8 = 1, rgb8 = 3, rgba8 = 4, rgba32_float = 16 };

struct image_load_options {
    image_format requested_format = image_format::rgba8;
    bool flip_vertically = false;
};

class COFFEE_API Image final {
public:
    Image() = default;
    Image(int width, int height, image_format format, std::vector<std::byte> pixels);

    [[nodiscard]] static Image load(const std::filesystem::path& path,
                                    image_load_options options = {});

    void flip_vertical();
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] image_format format() const noexcept;
    [[nodiscard]] int channels() const noexcept;
    [[nodiscard]] std::size_t row_bytes() const noexcept;
    [[nodiscard]] std::span<const std::byte> pixels() const noexcept;
    [[nodiscard]] std::span<std::byte> pixels() noexcept;

private:
    int width_ = 0;
    int height_ = 0;
    image_format format_ = image_format::rgba8;
    std::vector<std::byte> pixels_;
};

} // namespace coffee
