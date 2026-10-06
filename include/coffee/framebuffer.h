#pragma once

#include <coffee/api.h>

#include <coffee/texture.h>

#include <cstdint>
#include <memory>

namespace coffee {

struct framebuffer_config {
    int width = 1;
    int height = 1;
    texture_format color_format = texture_format::rgba8;
    bool depth_stencil = true;
    texture_filter filter = texture_filter::linear;
};

class COFFEE_API Framebuffer final {
public:
    explicit Framebuffer(framebuffer_config config);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    void bind() const noexcept;
    static void unbind() noexcept;
    void resize(int width, int height);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] const std::shared_ptr<Texture2D>& color_texture() const noexcept;
    [[nodiscard]] std::uint32_t native_handle() const noexcept;

private:
    void invalidate();
    void release() noexcept;

    framebuffer_config config_{};
    std::uint32_t handle_ = 0;
    std::uint32_t depth_stencil_handle_ = 0;
    std::shared_ptr<Texture2D> color_texture_;
};

} // namespace coffee
