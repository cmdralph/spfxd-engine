#pragma once

#include <coffee/api.h>
#include <coffee/math.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <variant>

namespace coffee {

enum class body_type_2d { static_body, kinematic_body, dynamic_body };

struct circle_shape_2d { float radius = 0.5f; };
struct box_shape_2d { vec2 half_extent{0.5f, 0.5f}; };
using collision_shape_2d = std::variant<circle_shape_2d, box_shape_2d>;

struct body_handle_2d {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return generation != 0; }
    [[nodiscard]] constexpr bool operator==(const body_handle_2d&) const noexcept = default;
};

struct rigid_body_config_2d {
    body_type_2d type = body_type_2d::dynamic_body;
    collision_shape_2d shape = box_shape_2d{};
    vec2 position{};
    vec2 velocity{};
    float mass = 1.0f;
    float restitution = 0.0f;
    float friction = 0.5f;
    float gravity_scale = 1.0f;
    bool sensor = false;
    std::uintptr_t user_data = 0;
};

struct contact_2d {
    body_handle_2d first{};
    body_handle_2d second{};
    vec2 normal{};
    float penetration = 0.0f;
    bool sensor = false;
};

struct ray_hit_2d {
    body_handle_2d body{};
    vec2 point{};
    vec2 normal{};
    float distance = 0.0f;
};

class COFFEE_API PhysicsWorld2D final {
public:
    explicit PhysicsWorld2D(vec2 gravity = {0.0f, -9.81f});
    ~PhysicsWorld2D();
    PhysicsWorld2D(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D(PhysicsWorld2D&&) noexcept;
    PhysicsWorld2D& operator=(PhysicsWorld2D&&) noexcept;

    [[nodiscard]] body_handle_2d create_body(const rigid_body_config_2d& config);
    bool destroy_body(body_handle_2d body) noexcept;
    [[nodiscard]] bool valid(body_handle_2d body) const noexcept;

    void step(float delta_seconds, int velocity_iterations = 8);
    void clear() noexcept;

    void set_gravity(vec2 gravity) noexcept;
    [[nodiscard]] vec2 gravity() const noexcept;
    [[nodiscard]] std::size_t body_count() const noexcept;
    [[nodiscard]] std::span<const contact_2d> contacts() const noexcept;

    [[nodiscard]] vec2 position(body_handle_2d body) const;
    void set_position(body_handle_2d body, vec2 position);
    [[nodiscard]] vec2 velocity(body_handle_2d body) const;
    void set_velocity(body_handle_2d body, vec2 velocity);
    void apply_force(body_handle_2d body, vec2 force);
    void apply_impulse(body_handle_2d body, vec2 impulse);
    [[nodiscard]] std::uintptr_t user_data(body_handle_2d body) const;

    [[nodiscard]] std::optional<ray_hit_2d> raycast(
        vec2 origin, vec2 direction, float maximum_distance) const;

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace coffee
