#include <coffee/physics_world_2d.h>
#include <coffee/error.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>
#include <type_traits>

namespace coffee {

namespace {

struct collision_result { vec2 normal{}; float penetration = 0.0f; };

std::optional<collision_result> collide_circle_circle(
    vec2 a_position, circle_shape_2d a, vec2 b_position, circle_shape_2d b) {
    const vec2 difference = b_position - a_position;
    const float distance_squared = dot(difference, difference);
    const float combined = a.radius + b.radius;
    if (distance_squared >= combined * combined) return std::nullopt;
    const float distance = std::sqrt(distance_squared);
    return collision_result{distance > 0.00001f ? difference / distance : vec2{1.0f, 0.0f}, combined - distance};
}

std::optional<collision_result> collide_box_box(
    vec2 a_position, box_shape_2d a, vec2 b_position, box_shape_2d b) {
    const vec2 difference = b_position - a_position;
    const float overlap_x = a.half_extent.x + b.half_extent.x - std::abs(difference.x);
    const float overlap_y = a.half_extent.y + b.half_extent.y - std::abs(difference.y);
    if (overlap_x <= 0.0f || overlap_y <= 0.0f) return std::nullopt;
    if (overlap_x < overlap_y) return collision_result{{difference.x < 0.0f ? -1.0f : 1.0f, 0.0f}, overlap_x};
    return collision_result{{0.0f, difference.y < 0.0f ? -1.0f : 1.0f}, overlap_y};
}

std::optional<collision_result> collide_circle_box(
    vec2 circle_position, circle_shape_2d circle, vec2 box_position, box_shape_2d box) {
    const vec2 local = circle_position - box_position;
    const vec2 closest{clamp(local.x, -box.half_extent.x, box.half_extent.x),
                       clamp(local.y, -box.half_extent.y, box.half_extent.y)};
    const vec2 difference = closest - local; // circle toward box
    const float distance_squared = dot(difference, difference);
    if (distance_squared > circle.radius * circle.radius) return std::nullopt;
    if (distance_squared > 0.0000001f) {
        const float distance = std::sqrt(distance_squared);
        return collision_result{difference / distance, circle.radius - distance};
    }
    const float x_depth = box.half_extent.x - std::abs(local.x);
    const float y_depth = box.half_extent.y - std::abs(local.y);
    if (x_depth < y_depth) return collision_result{{local.x > 0.0f ? -1.0f : 1.0f, 0.0f}, circle.radius + x_depth};
    return collision_result{{0.0f, local.y > 0.0f ? -1.0f : 1.0f}, circle.radius + y_depth};
}

} // namespace

struct PhysicsWorld2D::impl {
    struct body {
        rigid_body_config_2d config{};
        vec2 force{};
        float inverse_mass = 0.0f;
        std::uint32_t generation = 1;
        bool active = false;
    };

    vec2 gravity{0.0f, -9.81f};
    std::vector<body> bodies;
    std::vector<contact_2d> contacts;
    std::size_t active_count = 0;

    body& require(body_handle_2d handle) {
        if (!is_valid(handle)) throw error(error_code::invalid_argument, "Physics body handle is invalid or expired.");
        return bodies[handle.index];
    }
    const body& require(body_handle_2d handle) const {
        if (!is_valid(handle)) throw error(error_code::invalid_argument, "Physics body handle is invalid or expired.");
        return bodies[handle.index];
    }
    bool is_valid(body_handle_2d handle) const noexcept {
        return handle.generation != 0 && handle.index < bodies.size() && bodies[handle.index].active &&
               bodies[handle.index].generation == handle.generation;
    }
    body_handle_2d handle(std::size_t index) const noexcept {
        return {static_cast<std::uint32_t>(index), bodies[index].generation};
    }

    static std::optional<collision_result> collision(const body& a, const body& b) {
        if (const auto* first = std::get_if<circle_shape_2d>(&a.config.shape)) {
            if (const auto* second = std::get_if<circle_shape_2d>(&b.config.shape))
                return collide_circle_circle(a.config.position, *first, b.config.position, *second);
            return collide_circle_box(a.config.position, *first, b.config.position, std::get<box_shape_2d>(b.config.shape));
        }
        if (const auto* second = std::get_if<circle_shape_2d>(&b.config.shape)) {
            auto result = collide_circle_box(b.config.position, *second, a.config.position,
                                              std::get<box_shape_2d>(a.config.shape));
            if (result) result->normal = -result->normal;
            return result;
        }
        return collide_box_box(a.config.position, std::get<box_shape_2d>(a.config.shape),
                               b.config.position, std::get<box_shape_2d>(b.config.shape));
    }

    void solve(body& a, body& b, const collision_result& collision_value) {
        const float inverse_mass_sum = a.inverse_mass + b.inverse_mass;
        if (inverse_mass_sum == 0.0f || a.config.sensor || b.config.sensor) return;

        const vec2 relative_velocity = b.config.velocity - a.config.velocity;
        const float separating_velocity = dot(relative_velocity, collision_value.normal);
        float normal_impulse = 0.0f;
        if (separating_velocity < 0.0f) {
            const float restitution = std::min(a.config.restitution, b.config.restitution);
            normal_impulse = -(1.0f + restitution) * separating_velocity / inverse_mass_sum;
            const vec2 impulse = collision_value.normal * normal_impulse;
            a.config.velocity -= impulse * a.inverse_mass;
            b.config.velocity += impulse * b.inverse_mass;

            vec2 tangent = relative_velocity - collision_value.normal * separating_velocity;
            if (length(tangent) > 0.00001f) {
                tangent = normalized(tangent);
                float friction_impulse = -dot(relative_velocity, tangent) / inverse_mass_sum;
                const float coefficient = std::sqrt(a.config.friction * b.config.friction);
                friction_impulse = clamp(friction_impulse, -normal_impulse * coefficient, normal_impulse * coefficient);
                const vec2 friction = tangent * friction_impulse;
                a.config.velocity -= friction * a.inverse_mass;
                b.config.velocity += friction * b.inverse_mass;
            }
        }

        constexpr float slop = 0.001f;
        constexpr float correction_percent = 0.8f;
        const float magnitude = std::max(collision_value.penetration - slop, 0.0f) /
                                inverse_mass_sum * correction_percent;
        const vec2 correction = collision_value.normal * magnitude;
        a.config.position -= correction * a.inverse_mass;
        b.config.position += correction * b.inverse_mass;
    }
};

PhysicsWorld2D::PhysicsWorld2D(vec2 gravity) : impl_(std::make_unique<impl>()) { impl_->gravity = gravity; }
PhysicsWorld2D::~PhysicsWorld2D() = default;
PhysicsWorld2D::PhysicsWorld2D(PhysicsWorld2D&&) noexcept = default;
PhysicsWorld2D& PhysicsWorld2D::operator=(PhysicsWorld2D&&) noexcept = default;

body_handle_2d PhysicsWorld2D::create_body(const rigid_body_config_2d& config) {
    const auto valid_shape = std::visit([](const auto& shape) {
        if constexpr (std::is_same_v<std::decay_t<decltype(shape)>, circle_shape_2d>) return shape.radius > 0.0f;
        else return shape.half_extent.x > 0.0f && shape.half_extent.y > 0.0f;
    }, config.shape);
    if (!valid_shape || config.mass <= 0.0f || config.restitution < 0.0f || config.friction < 0.0f) {
        throw error(error_code::invalid_argument, "Rigid body shape and physical properties must be positive.");
    }
    std::size_t index = 0;
    for (; index < impl_->bodies.size(); ++index) if (!impl_->bodies[index].active) break;
    if (index == impl_->bodies.size()) impl_->bodies.emplace_back();
    auto& body = impl_->bodies[index];
    body.config = config;
    body.force = {};
    body.inverse_mass = config.type == body_type_2d::dynamic_body ? 1.0f / config.mass : 0.0f;
    body.active = true;
    if (body.generation == 0) body.generation = 1;
    ++impl_->active_count;
    return impl_->handle(index);
}

bool PhysicsWorld2D::destroy_body(body_handle_2d handle) noexcept {
    if (!valid(handle)) return false;
    auto& body = impl_->bodies[handle.index];
    body.active = false;
    body.generation = body.generation == std::numeric_limits<std::uint32_t>::max() ? 1 : body.generation + 1;
    --impl_->active_count;
    return true;
}

bool PhysicsWorld2D::valid(body_handle_2d body) const noexcept { return impl_ != nullptr && impl_->is_valid(body); }

void PhysicsWorld2D::step(float delta_seconds, int velocity_iterations) {
    if (delta_seconds <= 0.0f) return;
    if (velocity_iterations < 1) throw error(error_code::invalid_argument, "Physics iterations must be positive.");
    impl_->contacts.clear();
    for (auto& body : impl_->bodies) {
        if (!body.active) continue;
        if (body.config.type == body_type_2d::dynamic_body) {
            body.config.velocity += (impl_->gravity * body.config.gravity_scale + body.force * body.inverse_mass) * delta_seconds;
            body.config.position += body.config.velocity * delta_seconds;
        } else if (body.config.type == body_type_2d::kinematic_body) {
            body.config.position += body.config.velocity * delta_seconds;
        }
        body.force = {};
    }

    for (int iteration = 0; iteration < velocity_iterations; ++iteration) {
        for (std::size_t a_index = 0; a_index < impl_->bodies.size(); ++a_index) {
            auto& a = impl_->bodies[a_index];
            if (!a.active) continue;
            for (std::size_t b_index = a_index + 1; b_index < impl_->bodies.size(); ++b_index) {
                auto& b = impl_->bodies[b_index];
                if (!b.active || (a.config.type == body_type_2d::static_body &&
                                  b.config.type == body_type_2d::static_body)) continue;
                const auto collision_value = impl::collision(a, b);
                if (!collision_value) continue;
                if (iteration == 0) {
                    impl_->contacts.push_back({impl_->handle(a_index), impl_->handle(b_index),
                                               collision_value->normal, collision_value->penetration,
                                               a.config.sensor || b.config.sensor});
                }
                impl_->solve(a, b, *collision_value);
            }
        }
    }
}

void PhysicsWorld2D::clear() noexcept {
    for (auto& body : impl_->bodies) {
        body.active = false;
        body.generation = body.generation == std::numeric_limits<std::uint32_t>::max() ? 1 : body.generation + 1;
    }
    impl_->contacts.clear();
    impl_->active_count = 0;
}
void PhysicsWorld2D::set_gravity(vec2 value) noexcept { impl_->gravity = value; }
vec2 PhysicsWorld2D::gravity() const noexcept { return impl_->gravity; }
std::size_t PhysicsWorld2D::body_count() const noexcept { return impl_->active_count; }
std::span<const contact_2d> PhysicsWorld2D::contacts() const noexcept { return impl_->contacts; }
vec2 PhysicsWorld2D::position(body_handle_2d body) const { return impl_->require(body).config.position; }
void PhysicsWorld2D::set_position(body_handle_2d body, vec2 value) { impl_->require(body).config.position = value; }
vec2 PhysicsWorld2D::velocity(body_handle_2d body) const { return impl_->require(body).config.velocity; }
void PhysicsWorld2D::set_velocity(body_handle_2d body, vec2 value) { impl_->require(body).config.velocity = value; }
void PhysicsWorld2D::apply_force(body_handle_2d body, vec2 force) { impl_->require(body).force += force; }
void PhysicsWorld2D::apply_impulse(body_handle_2d body, vec2 impulse) {
    auto& value = impl_->require(body);
    if (value.config.type == body_type_2d::dynamic_body) value.config.velocity += impulse * value.inverse_mass;
}
std::uintptr_t PhysicsWorld2D::user_data(body_handle_2d body) const { return impl_->require(body).config.user_data; }

std::optional<ray_hit_2d> PhysicsWorld2D::raycast(vec2 origin, vec2 direction, float maximum_distance) const {
    if (maximum_distance <= 0.0f || length(direction) == 0.0f) return std::nullopt;
    direction = normalized(direction);
    std::optional<ray_hit_2d> nearest;
    for (std::size_t index = 0; index < impl_->bodies.size(); ++index) {
        const auto& body = impl_->bodies[index];
        if (!body.active) continue;
        float distance = std::numeric_limits<float>::infinity();
        vec2 normal{};
        if (const auto* circle = std::get_if<circle_shape_2d>(&body.config.shape)) {
            const vec2 offset = origin - body.config.position;
            const float b = dot(offset, direction);
            const float c = dot(offset, offset) - circle->radius * circle->radius;
            const float discriminant = b * b - c;
            if (discriminant >= 0.0f) {
                distance = -b - std::sqrt(discriminant);
                if (distance < 0.0f) distance = -b + std::sqrt(discriminant);
                if (distance >= 0.0f) normal = normalized(origin + direction * distance - body.config.position);
            }
        } else {
            const auto box = std::get<box_shape_2d>(body.config.shape);
            float near_time = 0.0f;
            float far_time = maximum_distance;
            for (int axis = 0; axis < 2; ++axis) {
                const float o = axis == 0 ? origin.x : origin.y;
                const float d = axis == 0 ? direction.x : direction.y;
                const float center = axis == 0 ? body.config.position.x : body.config.position.y;
                const float extent = axis == 0 ? box.half_extent.x : box.half_extent.y;
                if (std::abs(d) < 0.000001f) {
                    if (o < center - extent || o > center + extent) { near_time = maximum_distance + 1.0f; break; }
                } else {
                    float first = (center - extent - o) / d;
                    float second = (center + extent - o) / d;
                    if (first > second) std::swap(first, second);
                    if (first > near_time) {
                        near_time = first;
                        normal = axis == 0 ? vec2{d > 0.0f ? -1.0f : 1.0f, 0.0f}
                                           : vec2{0.0f, d > 0.0f ? -1.0f : 1.0f};
                    }
                    far_time = std::min(far_time, second);
                    if (near_time > far_time) break;
                }
            }
            if (near_time <= far_time) distance = near_time;
        }
        if (distance >= 0.0f && distance <= maximum_distance && (!nearest || distance < nearest->distance)) {
            nearest = ray_hit_2d{impl_->handle(index), origin + direction * distance, normal, distance};
        }
    }
    return nearest;
}

} // namespace coffee
