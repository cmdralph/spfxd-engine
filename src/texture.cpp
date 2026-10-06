#include <coffee/texture.h>
#include <coffee/error.h>

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace coffee {

namespace {

int channels(texture_format format) noexcept {
    switch (format) {
        case texture_format::r8: return 1;
        case texture_format::rgb8: return 3;
        case texture_format::rgba8: return 4;
    }
    return 4;
}

GLenum internal_format(texture_format format) noexcept {
    switch (format) {
        case texture_format::r8: return GL_R8;
        case texture_format::rgb8: return GL_RGB8;
        case texture_format::rgba8: return GL_RGBA8;
    }
    return GL_RGBA8;
}

GLenum data_format(texture_format format) noexcept {
    switch (format) {
        case texture_format::r8: return GL_RED;
        case texture_format::rgb8: return GL_RGB;
        case texture_format::rgba8: return GL_RGBA;
    }
    return GL_RGBA;
}

GLint filter(texture_filter value, bool mipmaps) noexcept {
    if (mipmaps) return value == texture_filter::nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
    return value == texture_filter::nearest ? GL_NEAREST : GL_LINEAR;
}

GLint wrap(texture_wrap value) noexcept {
    switch (value) {
        case texture_wrap::clamp_to_edge: return GL_CLAMP_TO_EDGE;
        case texture_wrap::repeat: return GL_REPEAT;
        case texture_wrap::mirrored_repeat: return GL_MIRRORED_REPEAT;
    }
    return GL_CLAMP_TO_EDGE;
}

std::size_t expected_size(int width, int height, texture_format format) {
    if (width <= 0 || height <= 0) {
        throw error(error_code::invalid_argument, "Texture dimensions must be positive.");
    }
    const auto size = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) *
                      static_cast<std::uint64_t>(channels(format));
    if (size > std::numeric_limits<std::size_t>::max()) {
        throw error(error_code::invalid_argument, "Texture dimensions are too large.");
    }
    return static_cast<std::size_t>(size);
}

} // namespace

Texture2D::Texture2D(int width, int height, texture_config config)
    : width_(width), height_(height), config_(config) {
    static_cast<void>(expected_size(width, height, config.format));
    glGenTextures(1, &handle_);
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter(config.min_filter, config.generate_mipmaps));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter(config.mag_filter, false));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap(config.wrap_x));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap(config.wrap_y));
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal_format(config.format)),
                 width, height, 0, data_format(config.format), GL_UNSIGNED_BYTE, nullptr);
}

Texture2D::Texture2D(int width, int height, std::span<const std::byte> pixels, texture_config config)
    : Texture2D(width, height, config) {
    set_data(pixels);
}

Texture2D::~Texture2D() { if (handle_ != 0) glDeleteTextures(1, &handle_); }
Texture2D::Texture2D(Texture2D&& other) noexcept
    : handle_(std::exchange(other.handle_, 0)), width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)), config_(other.config_) {}
Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        if (handle_ != 0) glDeleteTextures(1, &handle_);
        handle_ = std::exchange(other.handle_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        config_ = other.config_;
    }
    return *this;
}

std::shared_ptr<Texture2D> Texture2D::from_bmp(const std::filesystem::path& path, texture_config config) {
    struct surface_deleter {
        void operator()(SDL_Surface* surface) const noexcept { SDL_DestroySurface(surface); }
    };
    using surface_ptr = std::unique_ptr<SDL_Surface, surface_deleter>;

    surface_ptr loaded(SDL_LoadBMP(path.string().c_str()));
    if (!loaded) {
        throw error(error_code::file_io, "Could not load BMP image '" + path.string() + "': " + SDL_GetError());
    }
    surface_ptr converted(SDL_ConvertSurface(loaded.get(), SDL_PIXELFORMAT_RGBA32));
    if (!converted) {
        throw error(error_code::unsupported_format, "Could not convert BMP image to RGBA: " + std::string(SDL_GetError()));
    }

    config.format = texture_format::rgba8;
    const auto byte_count = expected_size(converted->w, converted->h, config.format);
    const std::size_t row_bytes = static_cast<std::size_t>(converted->w) * 4;
    std::vector<std::byte> contiguous(byte_count);
    const auto* source = static_cast<const std::byte*>(converted->pixels);
    for (int row = 0; row < converted->h; ++row) {
        std::memcpy(contiguous.data() + static_cast<std::size_t>(row) * row_bytes,
                    source + static_cast<std::size_t>(row) * static_cast<std::size_t>(converted->pitch),
                    row_bytes);
    }
    return std::make_shared<Texture2D>(converted->w, converted->h, contiguous, config);
}

std::shared_ptr<Texture2D> Texture2D::white() {
    static std::weak_ptr<Texture2D> cached;
    if (auto existing = cached.lock()) return existing;
    constexpr std::array<std::byte, 4> pixel{
        std::byte{255}, std::byte{255}, std::byte{255}, std::byte{255}
    };
    auto created = std::make_shared<Texture2D>(1, 1, pixel);
    cached = created;
    return created;
}

std::shared_ptr<Texture2D> Texture2D::from_file(
    const std::filesystem::path& path, texture_config config, bool flip_vertically) {
    return from_image(Image::load(path, {.requested_format = image_format::rgba8,
                                         .flip_vertically = flip_vertically}), config);
}

std::shared_ptr<Texture2D> Texture2D::from_image(const Image& image, texture_config config) {
    if (image.empty()) throw error(error_code::invalid_argument, "Cannot create a texture from an empty image.");
    switch (image.format()) {
        case image_format::r8: config.format = texture_format::r8; break;
        case image_format::rgb8: config.format = texture_format::rgb8; break;
        case image_format::rgba8: config.format = texture_format::rgba8; break;
        case image_format::rgba32_float:
            throw error(error_code::unsupported_format, "Floating-point CPU images are not supported by Texture2D yet.");
    }
    return std::make_shared<Texture2D>(image.width(), image.height(), image.pixels(), config);
}

void Texture2D::bind(std::uint32_t slot) const {
    if (slot >= 32) throw error(error_code::invalid_argument, "Texture slot must be below 32.");
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, handle_);
}
void Texture2D::unbind(std::uint32_t slot) {
    if (slot >= 32) return;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void Texture2D::set_data(std::span<const std::byte> pixels) {
    const auto required = expected_size(width_, height_, config_.format);
    if (pixels.size_bytes() != required) {
        throw error(error_code::invalid_argument, "Texture pixel data does not match its dimensions and format.");
    }
    bind();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, data_format(config_.format),
                    GL_UNSIGNED_BYTE, pixels.data());
    if (config_.generate_mipmaps) glGenerateMipmap(GL_TEXTURE_2D);
}
int Texture2D::width() const noexcept { return width_; }
int Texture2D::height() const noexcept { return height_; }
texture_format Texture2D::format() const noexcept { return config_.format; }
std::uint32_t Texture2D::native_handle() const noexcept { return handle_; }

} // namespace coffee
