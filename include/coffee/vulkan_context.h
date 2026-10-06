#pragma once

#include <coffee/api.h>
#include <coffee/config.h>
#include <coffee/math.h>
#include <coffee/window.h>

#include <cstdint>
#include <memory>
#include <string>

namespace coffee {

struct vulkan_config {
    std::string application_name = "Coffee application";
    bool vsync = true;
    std::uint32_t frames_in_flight = 2;
};

class COFFEE_VULKAN_API VulkanContext final {
public:
    explicit VulkanContext(Window& window, vulkan_config config = {});
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) noexcept;
    VulkanContext& operator=(VulkanContext&&) noexcept;

    // Returns false while the swapchain is unavailable, such as when minimized.
    [[nodiscard]] bool begin_frame(color clear_color = {0.04f, 0.05f, 0.075f, 1.0f});
    void end_frame();
    void request_resize() noexcept;
    void wait_idle();

    [[nodiscard]] std::string device_name() const;
    [[nodiscard]] std::uint32_t api_version() const noexcept;
    [[nodiscard]] std::uintptr_t native_instance() const noexcept;
    [[nodiscard]] std::uintptr_t native_physical_device() const noexcept;
    [[nodiscard]] std::uintptr_t native_device() const noexcept;

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace coffee
