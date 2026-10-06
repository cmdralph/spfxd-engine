#pragma once

#include <coffee/api.h>
#include <coffee/math.h>
#include <coffee/texture.h>

#include <cstddef>
#include <memory>

namespace coffee {

struct sprite_batch_stats {
    std::size_t draw_calls = 0;
    std::size_t quad_count = 0;
};

class COFFEE_API SpriteBatch final {
public:
    explicit SpriteBatch(std::size_t maximum_quads = 10'000);
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;
    SpriteBatch(SpriteBatch&& other) noexcept;
    SpriteBatch& operator=(SpriteBatch&& other) noexcept;

    void begin(const mat4& view_projection = mat4::identity());

    void draw_rect(
        vec2 position,
        vec2 size,
        color tint = {1.0f, 1.0f, 1.0f, 1.0f},
        float rotation_radians = 0.0f,
        vec2 origin = {0.5f, 0.5f});

    void draw_texture(
        const std::shared_ptr<Texture2D>& texture,
        vec2 position,
        vec2 size,
        color tint = {1.0f, 1.0f, 1.0f, 1.0f},
        vec2 uv_min = {0.0f, 0.0f},
        vec2 uv_max = {1.0f, 1.0f},
        float rotation_radians = 0.0f,
        vec2 origin = {0.5f, 0.5f});

    void end();
    void flush();

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] sprite_batch_stats stats() const noexcept;

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace coffee
