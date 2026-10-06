# Coffee architecture

Coffee 0.5 has two binary layers:

```text
Application
    |
    +-- Coffee.dll
    |     Application, window, input, assets, logging, images, math, physics
    |     OpenGL renderer and resources
    |     private SDL3 + GLAD + optional stb_image
    |
    +-- CoffeeVulkan.dll (optional)
          Vulkan instance, device, swapchain, and frame lifecycle
```

## ABI boundary

`coffee/api.h` is the only visibility policy. Public classes and free functions
are explicitly exported; automatic export-all behavior is disabled. SDL, GLAD,
and stb_image are private implementation dependencies and their include paths
do not propagate to an application.

Public headers may use the C++ standard library. That keeps Coffee pleasant to
use, but means SDK releases must name their architecture and supported compiler
runtime. ABI-breaking public-layout changes require a major version change.

## Ownership

- `Window` owns its SDL window and optional OpenGL context.
- `Application` owns one window, renderer, timer, and optional fixed-step loop.
- `AssetLocator` owns an ordered set of normalized search roots.
- `VulkanContext` owns Vulkan handles for one Vulkan window.
- GPU objects destroy native handles and transfer ownership when moved.
- `VertexArray` retains shared ownership of attached buffers.
- `Mesh` owns vertex/index buffers and its vertex array.
- `Image` owns CPU pixels; `Texture2D` owns GPU pixels.
- `PhysicsWorld2D` owns slots exposed through generation-checked handles.

## Backend boundary

OpenGL is Coffee's compact built-in renderer. Vulkan is a companion module,
not a compile-time branch spread through every OpenGL class. Platform, math,
image, mesh-data, timing, and physics systems are shared. Backend resources are
allowed to model their real APIs until common workloads justify a higher-level
render interface.

## Native access

`native_window_handle()` and `native_gl_context()` remain opaque. Raw OpenGL
extensions can be loaded through `coffee::native::opengl_proc_address()` using
an application's chosen loader. Coffee's internal GLAD globals are not exported
as part of the stable ABI.

## Threading

Window and event operations run on the main thread. OpenGL resources must be
created and destroyed while their context is current. The physics world is
single-threaded and deterministic for a fixed body order/timestep. File/image
decoding may run on workers; GPU upload remains render-thread work. Logging is
thread-safe, invokes the configured callback synchronously, and never lets an
exception cross the logging boundary.

## Runtime size policy

The default embedded SDL build disables subsystems Coffee does not currently
expose: audio, camera, SDL GPU/render, haptics, HID/gamepads, power, sensors,
dialogs, notifications, process launching, tray APIs, and OpenGL ES. This keeps Coffee.dll focused while preserving SDL's
windowing, event, clipboard, filesystem, OpenGL, and Vulkan-surface support.
Set `COFFEE_LEAN_SDL=OFF` when deliberately using those native SDL systems.
