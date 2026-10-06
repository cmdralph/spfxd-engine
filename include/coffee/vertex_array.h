#pragma once

#include <coffee/api.h>
#include <coffee/buffer.h>
#include <coffee/render_types.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace coffee {

struct buffer_element {
    std::string name;
    shader_data_type type = shader_data_type::float_1;
    bool normalized = false;
    std::size_t offset = 0;
};

class COFFEE_API BufferLayout final {
public:
    BufferLayout() = default;
    BufferLayout(std::initializer_list<buffer_element> elements);

    [[nodiscard]] const std::vector<buffer_element>& elements() const noexcept;
    [[nodiscard]] std::size_t stride() const noexcept;

private:
    void calculate() noexcept;
    std::vector<buffer_element> elements_;
    std::size_t stride_ = 0;
};

class COFFEE_API VertexArray final {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void bind() const noexcept;
    static void unbind() noexcept;
    void add_vertex_buffer(std::shared_ptr<VertexBuffer> buffer, const BufferLayout& layout);
    void set_index_buffer(std::shared_ptr<IndexBuffer> buffer);

    [[nodiscard]] const std::shared_ptr<IndexBuffer>& index_buffer() const noexcept;
    [[nodiscard]] std::uint32_t native_handle() const noexcept;

private:
    std::uint32_t handle_ = 0;
    std::uint32_t next_attribute_ = 0;
    std::vector<std::shared_ptr<VertexBuffer>> vertex_buffers_;
    std::shared_ptr<IndexBuffer> index_buffer_;
};

} // namespace coffee
