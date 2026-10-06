#pragma once

#include <coffee/api.h>

#include <coffee/render_types.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace coffee {

class COFFEE_API VertexBuffer final {
public:
    explicit VertexBuffer(std::size_t size, buffer_usage usage = buffer_usage::dynamic_draw);
    VertexBuffer(std::span<const std::byte> data, buffer_usage usage = buffer_usage::static_draw);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    void bind() const noexcept;
    static void unbind() noexcept;
    void set_data(std::span<const std::byte> data, std::size_t offset = 0);

    template<typename T>
    void set_data(std::span<const T> data, std::size_t offset = 0) {
        set_data(std::as_bytes(data), offset);
    }

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::uint32_t native_handle() const noexcept;

private:
    std::uint32_t handle_ = 0;
    std::size_t size_ = 0;
};

class COFFEE_API IndexBuffer final {
public:
    explicit IndexBuffer(std::span<const std::uint32_t> indices,
                         buffer_usage usage = buffer_usage::static_draw);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    void bind() const noexcept;
    static void unbind() noexcept;
    [[nodiscard]] std::size_t count() const noexcept;
    [[nodiscard]] std::uint32_t native_handle() const noexcept;

private:
    std::uint32_t handle_ = 0;
    std::size_t count_ = 0;
};

} // namespace coffee
