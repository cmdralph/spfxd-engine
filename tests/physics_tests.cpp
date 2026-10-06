#include <coffee/physics_world_2d.h>

#include <cmath>
#include <iostream>

int main() {
    coffee::PhysicsWorld2D world;
    const auto floor = world.create_body({
        .type = coffee::body_type_2d::static_body,
        .shape = coffee::box_shape_2d{{5.0f, 0.5f}},
        .position = {0.0f, -0.5f}
    });
    const auto box = world.create_body({
        .shape = coffee::box_shape_2d{{0.5f, 0.5f}},
        .position = {0.0f, 4.0f},
        .friction = 0.6f
    });
    for (int step = 0; step < 600; ++step) world.step(1.0f / 120.0f);
    const auto position = world.position(box);
    if (!world.valid(floor) || !world.valid(box) || std::abs(position.y - 0.5f) > 0.03f) {
        std::cerr << "Physics rest position failed: " << position.y << '\n';
        return 1;
    }
    const auto hit = world.raycast({0.0f, 3.0f}, {0.0f, -1.0f}, 10.0f);
    if (!hit || hit->body != box) {
        std::cerr << "Physics raycast failed.\n";
        return 1;
    }
    std::cout << "All Coffee physics tests passed.\n";
    return 0;
}

