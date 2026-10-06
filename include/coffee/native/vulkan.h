#pragma once

#include <coffee/config.h>

#if !COFFEE_HAS_VULKAN_MODULE
#error "CoffeeVulkan was not built. Configure with COFFEE_BUILD_VULKAN_MODULE=ON."
#endif

#include <vulkan/vulkan.h>
