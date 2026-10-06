#pragma once

#include <coffee/api.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

#include <coffee/image.h>

namespace coffee {

enum class texture_format { r8, rgb8, rgba8 };
enum class texture_filter { nearest, linear };
enum class texture_wrap { clamp_to_edge, repeat, mirrored_repeat };

struct texture_config {
    texture_format format = texture_format::rgba8;
    texture_filter min_filter = texture_filter::linear;
    texture_filter mag_filter = texture_filter::linear;
    texture_wrap wrap_x = texture_wrap::clamp_to_edge;
    texture_wrap wrap_y = texture_wrap::clamp_to_edge;
    bool generate_mipmaps = false;
};

class COFFEE_API Texture2D final {
public:
    Texture2D(int width, int height, texture_config config = {});
    Texture2D(int width, int height, std::span<const std::byte> pixels,
              texture_config config = {});
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    [[nodiscard]] static std::shared_ptr<Texture2D> from_bmp(
        const std::filesystem::path& path,
        texture_config config = {});
    [[nodiscard]] static std::shared_ptr<Texture2D> from_file(
        const std::filesystem::path& path,
        texture_config config = {},
        bool flip_vertically = true);
    [[nodiscard]] static std::shared_ptr<Texture2D> from_image(
        const Image& image,
        texture_config config = {});
    [[nodiscard]] static std::shared_ptr<Texture2D> white();

    void bind(std::uint32_t slot = 0) const;
    static void unbind(std::uint32_t slot = 0);
    void set_data(std::span<const std::byte> pixels);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] texture_format format() const noexcept;
    [[nodiscard]] std::uint32_t native_handle() const noexcept;

private:
    std::uint32_t handle_ = 0;
    int width_ = 0;
    int height_ = 0;
    texture_config config_{};
};

} // namespace coffee
