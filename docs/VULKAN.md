# Optional Vulkan module

Vulkan is built as `CoffeeVulkan.dll`, linked alongside the core `Coffee.dll`.
The core owns SDL and exposes two narrow operations to the module: the required
instance-extension list and creation of a surface for a Vulkan window. SDL
headers and symbols therefore stay private even when Vulkan is enabled.

Build it with the standalone `windows-msvc-vulkan` preset documented in
`BUILDING.md`.

```cpp
#include <coffee/coffee.h>

int main() {
    coffee::Window window({
        .title = "Vulkan application",
        .width = 1280,
        .height = 720,
        .backend = coffee::graphics_backend::vulkan
    });

    coffee::VulkanContext graphics(window);

    while (window.is_open()) {
        window.poll_events();
        if (graphics.begin_frame({0.05f, 0.08f, 0.14f, 1.0f})) {
            graphics.end_frame();
        }
    }
}
```

Implemented:

- instance extensions and surface creation
- physical/logical device and queue selection
- swapchain formats, present modes, images, and views
- clear render pass and framebuffers
- command buffers, semaphores, fences, acquisition, and presentation
- resize/out-of-date swapchain recreation
- native Vulkan handles for advanced integration

Not yet presented as complete:

- graphics/compute pipeline abstractions
- Vulkan buffers, images, allocation, and uploads
- descriptors, reflection, and SPIR-V compilation
- Vulkan mesh or sprite rendering

The boundary is intentional: Vulkan resource and synchronization models should
not be disguised as OpenGL objects with renamed methods.
