#include <coffee/vertex_array.h>
#include <coffee/error.h>

#include <glad/gl.h>

#include <limits>
#include <utility>

namespace coffee {

BufferLayout::BufferLayout(std::initializer_list<buffer_element> elements) : elements_(elements) { calculate(); }
const std::vector<buffer_element>& BufferLayout::elements() const noexcept { return elements_; }
std::size_t BufferLayout::stride() const noexcept { return stride_; }
void BufferLayout::calculate() noexcept {
    std::size_t offset = 0;
    stride_ = 0;
    for (auto& element : elements_) {
        element.offset = offset;
        const auto size = shader_data_size(element.type);
        offset += size;
        stride_ += size;
    }
}

VertexArray::VertexArray() { glGenVertexArrays(1, &handle_); }
VertexArray::~VertexArray() { if (handle_ != 0) glDeleteVertexArrays(1, &handle_); }
VertexArray::VertexArray(VertexArray&& other) noexcept
    : handle_(std::exchange(other.handle_, 0)),
      next_attribute_(std::exchange(other.next_attribute_, 0)),
      vertex_buffers_(std::move(other.vertex_buffers_)),
      index_buffer_(std::move(other.index_buffer_)) {}
VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (handle_ != 0) glDeleteVertexArrays(1, &handle_);
        handle_ = std::exchange(other.handle_, 0);
        next_attribute_ = std::exchange(other.next_attribute_, 0);
        vertex_buffers_ = std::move(other.vertex_buffers_);
        index_buffer_ = std::move(other.index_buffer_);
    }
    return *this;
}
void VertexArray::bind() const noexcept { glBindVertexArray(handle_); }
void VertexArray::unbind() noexcept { glBindVertexArray(0); }

void VertexArray::add_vertex_buffer(std::shared_ptr<VertexBuffer> buffer, const BufferLayout& layout) {
    if (!buffer) throw error(error_code::invalid_argument, "Vertex buffer cannot be null.");
    if (layout.elements().empty() || layout.stride() == 0) {
        throw error(error_code::invalid_argument, "Vertex layout cannot be empty.");
    }
    if (layout.stride() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw error(error_code::invalid_argument, "Vertex stride is too large.");
    }

    bind();
    buffer->bind();
    for (const auto& element : layout.elements()) {
        const auto count = shader_data_components(element.type);
        const auto pointer = reinterpret_cast<const void*>(element.offset);
        glEnableVertexAttribArray(next_attribute_);

        switch (element.type) {
            case shader_data_type::int_1:
            case shader_data_type::int_2:
            case shader_data_type::int_3:
            case shader_data_type::int_4:
                glVertexAttribIPointer(next_attribute_, count, GL_INT,
                                       static_cast<GLsizei>(layout.stride()), pointer);
                break;
            case shader_data_type::unsigned_byte_4:
                glVertexAttribPointer(next_attribute_, count, GL_UNSIGNED_BYTE,
                                      element.normalized ? GL_TRUE : GL_FALSE,
                                      static_cast<GLsizei>(layout.stride()), pointer);
                break;
            default:
                glVertexAttribPointer(next_attribute_, count, GL_FLOAT,
                                      element.normalized ? GL_TRUE : GL_FALSE,
                                      static_cast<GLsizei>(layout.stride()), pointer);
                break;
        }
        ++next_attribute_;
    }
    vertex_buffers_.push_back(std::move(buffer));
}

void VertexArray::set_index_buffer(std::shared_ptr<IndexBuffer> buffer) {
    if (!buffer) throw error(error_code::invalid_argument, "Index buffer cannot be null.");
    bind();
    buffer->bind();
    index_buffer_ = std::move(buffer);
}
const std::shared_ptr<IndexBuffer>& VertexArray::index_buffer() const noexcept { return index_buffer_; }
std::uint32_t VertexArray::native_handle() const noexcept { return handle_; }

} // namespace coffee
