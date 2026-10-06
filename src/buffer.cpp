#include <coffee/buffer.h>
#include <coffee/error.h>

#include <glad/gl.h>

#include <limits>
#include <utility>

namespace coffee {

namespace {

GLenum to_gl(buffer_usage usage) noexcept {
    switch (usage) {
        case buffer_usage::static_draw: return GL_STATIC_DRAW;
        case buffer_usage::dynamic_draw: return GL_DYNAMIC_DRAW;
        case buffer_usage::stream_draw: return GL_STREAM_DRAW;
    }
    return GL_STATIC_DRAW;
}

GLsizeiptr checked_size(std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())) {
        throw error(error_code::invalid_argument, "GPU buffer is too large for this OpenGL implementation.");
    }
    return static_cast<GLsizeiptr>(size);
}

} // namespace

VertexBuffer::VertexBuffer(std::size_t size, buffer_usage usage) : size_(size) {
    if (size == 0) throw error(error_code::invalid_argument, "Vertex buffer size must be positive.");
    glGenBuffers(1, &handle_);
    bind();
    glBufferData(GL_ARRAY_BUFFER, checked_size(size), nullptr, to_gl(usage));
}

VertexBuffer::VertexBuffer(std::span<const std::byte> data, buffer_usage usage)
    : size_(data.size_bytes()) {
    if (data.empty()) throw error(error_code::invalid_argument, "Vertex buffer data cannot be empty.");
    glGenBuffers(1, &handle_);
    bind();
    glBufferData(GL_ARRAY_BUFFER, checked_size(data.size_bytes()), data.data(), to_gl(usage));
}

VertexBuffer::~VertexBuffer() { if (handle_ != 0) glDeleteBuffers(1, &handle_); }
VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : handle_(std::exchange(other.handle_, 0)), size_(std::exchange(other.size_, 0)) {}
VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        if (handle_ != 0) glDeleteBuffers(1, &handle_);
        handle_ = std::exchange(other.handle_, 0);
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}
void VertexBuffer::bind() const noexcept { glBindBuffer(GL_ARRAY_BUFFER, handle_); }
void VertexBuffer::unbind() noexcept { glBindBuffer(GL_ARRAY_BUFFER, 0); }
void VertexBuffer::set_data(std::span<const std::byte> data, std::size_t offset) {
    if (offset > size_ || data.size_bytes() > size_ - offset) {
        throw error(error_code::invalid_argument, "Vertex buffer update is outside the allocated storage.");
    }
    bind();
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), checked_size(data.size_bytes()), data.data());
}
std::size_t VertexBuffer::size() const noexcept { return size_; }
std::uint32_t VertexBuffer::native_handle() const noexcept { return handle_; }

IndexBuffer::IndexBuffer(std::span<const std::uint32_t> indices, buffer_usage usage)
    : count_(indices.size()) {
    if (indices.empty()) throw error(error_code::invalid_argument, "Index buffer data cannot be empty.");
    glGenBuffers(1, &handle_);
    bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, checked_size(indices.size_bytes()), indices.data(), to_gl(usage));
}
IndexBuffer::~IndexBuffer() { if (handle_ != 0) glDeleteBuffers(1, &handle_); }
IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : handle_(std::exchange(other.handle_, 0)), count_(std::exchange(other.count_, 0)) {}
IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
    if (this != &other) {
        if (handle_ != 0) glDeleteBuffers(1, &handle_);
        handle_ = std::exchange(other.handle_, 0);
        count_ = std::exchange(other.count_, 0);
    }
    return *this;
}
void IndexBuffer::bind() const noexcept { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_); }
void IndexBuffer::unbind() noexcept { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
std::size_t IndexBuffer::count() const noexcept { return count_; }
std::uint32_t IndexBuffer::native_handle() const noexcept { return handle_; }

} // namespace coffee
