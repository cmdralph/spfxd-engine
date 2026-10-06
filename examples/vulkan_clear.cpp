#include <coffee/coffee.h>

#include <exception>
#include <cmath>
#include <iostream>

int main() {
    try {
        coffee::Window window({
            .title = "Coffee Vulkan",
            .width = 1100,
            .height = 700,
            .backend = coffee::graphics_backend::vulkan
        });
        coffee::VulkanContext vulkan(window);
        coffee::frame_timer timer;
        std::cout << "Vulkan device: " << vulkan.device_name() << '\n';

        while (window.is_open()) {
            window.poll_events();
            timer.tick();
            if (window.input().key_pressed(coffee::key::escape)) window.close();
            for (const auto& event : window.events()) {
                if (event.type == coffee::event_type::framebuffer_resized) vulkan.request_resize();
            }
            const float pulse = 0.08f + static_cast<float>(std::sin(timer.elapsed_seconds())) * 0.025f;
            if (vulkan.begin_frame({pulse, 0.10f, 0.16f, 1.0f})) vulkan.end_frame();
        }
        vulkan.wait_idle();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Coffee Vulkan error: " << exception.what() << '\n';
        return 1;
    }
}
