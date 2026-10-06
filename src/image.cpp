#include <coffee/image.h>
#include <coffee/config.h>
#include <coffee/error.h>

#include <SDL3/SDL.h>

#if COFFEE_HAS_STB_IMAGE
#include <stb_image.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <memory>
#include <string>

namespace coffee {

namespace {

std::size_t bytes_per_pixel(image_format format) noexcept {
    return static_cast<std::size_t>(format);
}

std::size_t checked_byte_count(int width, int height, image_format format) {
    if (width <= 0 || height <= 0) {
        throw error(error_code::invalid_argument, "Image dimensions must be positive.");
    }
    const auto result = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) * bytes_per_pixel(format);
    if (result > std::numeric_limits<std::size_t>::max()) {
        throw error(error_code::invalid_argument, "Image dimensions are too large.");
    }
    return static_cast<std::size_t>(result);
}

Image load_bmp(const std::filesystem::path& path) {
    struct deleter { void operator()(SDL_Surface* value) const noexcept { SDL_DestroySurface(value); } };
    using surface_ptr = std::unique_ptr<SDL_Surface, deleter>;
    surface_ptr source(SDL_LoadBMP(path.string().c_str()));
    if (!source) throw error(error_code::file_io, "Could not load image '" + path.string() + "': " + SDL_GetError());
    surface_ptr converted(SDL_ConvertSurface(source.get(), SDL_PIXELFORMAT_RGBA32));
    if (!converted) throw error(error_code::unsupported_format, "Could not convert BMP to RGBA: " + std::string(SDL_GetError()));

    const std::size_t row_size = static_cast<std::size_t>(converted->w) * 4;
    std::vector<std::byte> pixels(checked_byte_count(converted->w, converted->h, image_format::rgba8));
    const auto* input = static_cast<const std::byte*>(converted->pixels);
    for (int row = 0; row < converted->h; ++row) {
        std::memcpy(pixels.data() + static_cast<std::size_t>(row) * row_size,
                    input + static_cast<std::size_t>(row) * static_cast<std::size_t>(converted->pitch), row_size);
    }
    return {converted->w, converted->h, image_format::rgba8, std::move(pixels)};
}

} // namespace

Image::Image(int width, int height, image_format format, std::vector<std::byte> pixels)
    : width_(width), height_(height), format_(format), pixels_(std::move(pixels)) {
    if (pixels_.size() != checked_byte_count(width, height, format)) {
        throw error(error_code::invalid_argument, "Image pixel data size does not match its dimensions and format.");
    }
}

Image Image::load(const std::filesystem::path& path, image_load_options options) {
    if (path.empty()) throw error(error_code::invalid_argument, "Image path cannot be empty.");
    if (options.requested_format == image_format::rgba32_float) {
        throw error(error_code::unsupported_format,
                    "Use an 8-bit requested image format; HDR float decoding is reserved for a later API.");
    }
#if COFFEE_HAS_STB_IMAGE
    const int desired_channels = options.requested_format == image_format::r8 ? 1
        : options.requested_format == image_format::rgb8 ? 3 : 4;
    int width = 0;
    int height = 0;
    int source_channels = 0;
    stbi_uc* decoded = stbi_load(path.string().c_str(), &width, &height, &source_channels, desired_channels);
    if (decoded == nullptr) {
        throw error(error_code::unsupported_format,
                    "Could not decode image '" + path.string() + "': " + stbi_failure_reason());
    }
    const image_format output_format = desired_channels == 1 ? image_format::r8
        : desired_channels == 3 ? image_format::rgb8 : image_format::rgba8;
    const std::size_t size = checked_byte_count(width, height, output_format);
    std::vector<std::byte> pixels(size);
    std::memcpy(pixels.data(), decoded, size);
    stbi_image_free(decoded);
    Image result(width, height, output_format, std::move(pixels));
#else
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (extension != ".bmp") {
        throw error(error_code::unsupported_format,
                    "This Coffee build supports BMP only. Add deps/stb/stb_image.h for PNG, JPEG, TGA, PSD, GIF, HDR and PIC.");
    }
    Image result = load_bmp(path);
    if (options.requested_format != image_format::rgba8) {
        const int output_channels = options.requested_format == image_format::r8 ? 1 : 3;
        std::vector<std::byte> converted(
            static_cast<std::size_t>(result.width_) * static_cast<std::size_t>(result.height_) *
            static_cast<std::size_t>(output_channels));
        for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(result.width_) * static_cast<std::size_t>(result.height_); ++pixel) {
            const auto red = std::to_integer<unsigned char>(result.pixels_[pixel * 4]);
            const auto green = std::to_integer<unsigned char>(result.pixels_[pixel * 4 + 1]);
            const auto blue = std::to_integer<unsigned char>(result.pixels_[pixel * 4 + 2]);
            if (output_channels == 1) {
                converted[pixel] = std::byte{static_cast<unsigned char>(
                    (static_cast<unsigned>(red) * 54u + static_cast<unsigned>(green) * 183u +
                     static_cast<unsigned>(blue) * 19u) / 256u)};
            } else {
                converted[pixel * 3] = std::byte{red};
                converted[pixel * 3 + 1] = std::byte{green};
                converted[pixel * 3 + 2] = std::byte{blue};
            }
        }
        result.format_ = options.requested_format;
        result.pixels_ = std::move(converted);
    }
#endif
    if (options.flip_vertically) result.flip_vertical();
    return result;
}

void Image::flip_vertical() {
    if (empty()) return;
    const std::size_t stride = row_bytes();
    std::vector<std::byte> temporary(stride);
    for (int row = 0; row < height_ / 2; ++row) {
        auto* top = pixels_.data() + static_cast<std::size_t>(row) * stride;
        auto* bottom = pixels_.data() + static_cast<std::size_t>(height_ - row - 1) * stride;
        std::memcpy(temporary.data(), top, stride);
        std::memcpy(top, bottom, stride);
        std::memcpy(bottom, temporary.data(), stride);
    }
}

bool Image::empty() const noexcept { return pixels_.empty(); }
int Image::width() const noexcept { return width_; }
int Image::height() const noexcept { return height_; }
image_format Image::format() const noexcept { return format_; }
int Image::channels() const noexcept { return format_ == image_format::rgba32_float ? 4 : static_cast<int>(format_); }
std::size_t Image::row_bytes() const noexcept { return static_cast<std::size_t>(width_) * bytes_per_pixel(format_); }
std::span<const std::byte> Image::pixels() const noexcept { return pixels_; }
std::span<std::byte> Image::pixels() noexcept { return pixels_; }

} // namespace coffee
