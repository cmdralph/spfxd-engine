#include <coffee/framebuffer.h>
#include <coffee/error.h>

#include <glad/gl.h>

#include <utility>

namespace coffee {

Framebuffer::Framebuffer(framebuffer_config config) : config_(config) {
    if (config.width <= 0 || config.height <= 0) {
        throw error(error_code::invalid_argument, "Framebuffer dimensions must be positive.");
    }
    invalidate();
}
Framebuffer::~Framebuffer() { release(); }
Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : config_(other.config_), handle_(std::exchange(other.handle_, 0)),
      depth_stencil_handle_(std::exchange(other.depth_stencil_handle_, 0)),
      color_texture_(std::move(other.color_texture_)) {}
Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        release();
        config_ = other.config_;
        handle_ = std::exchange(other.handle_, 0);
        depth_stencil_handle_ = std::exchange(other.depth_stencil_handle_, 0);
        color_texture_ = std::move(other.color_texture_);
    }
    return *this;
}
void Framebuffer::bind() const noexcept {
    glBindFramebuffer(GL_FRAMEBUFFER, handle_);
    glViewport(0, 0, config_.width, config_.height);
}
void Framebuffer::unbind() noexcept { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
void Framebuffer::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (config_.width == width && config_.height == height) return;
    config_.width = width;
    config_.height = height;
    invalidate();
}
int Framebuffer::width() const noexcept { return config_.width; }
int Framebuffer::height() const noexcept { return config_.height; }
const std::shared_ptr<Texture2D>& Framebuffer::color_texture() const noexcept { return color_texture_; }
std::uint32_t Framebuffer::native_handle() const noexcept { return handle_; }

void Framebuffer::invalidate() {
    release();
    glGenFramebuffers(1, &handle_);
    glBindFramebuffer(GL_FRAMEBUFFER, handle_);

    texture_config texture{};
    texture.format = config_.color_format;
    texture.min_filter = config_.filter;
    texture.mag_filter = config_.filter;
    color_texture_ = std::make_shared<Texture2D>(config_.width, config_.height, texture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           color_texture_->native_handle(), 0);

    if (config_.depth_stencil) {
        glGenRenderbuffers(1, &depth_stencil_handle_);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_handle_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, config_.width, config_.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, depth_stencil_handle_);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        release();
        throw error(error_code::graphics_initialization, "OpenGL framebuffer is incomplete.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::release() noexcept {
    color_texture_.reset();
    if (depth_stencil_handle_ != 0) {
        glDeleteRenderbuffers(1, &depth_stencil_handle_);
        depth_stencil_handle_ = 0;
    }
    if (handle_ != 0) {
        glDeleteFramebuffers(1, &handle_);
        handle_ = 0;
    }
}

} // namespace coffee
