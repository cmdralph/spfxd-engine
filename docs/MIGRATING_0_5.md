# Migrating from Coffee 0.4 to 0.5

Coffee 0.5 is source-compatible with ordinary 0.4 window/render loops. Rebuild
applications because the DLL's exported C++ ABI has changed.

## Version and package lookup

Update installed-package requirements:

```cmake
find_package(Coffee 0.5 CONFIG REQUIRED)
```

You can replace manual linking and DLL copying with:

```cmake
coffee_add_executable(my_app src/main.cpp)
```

or add `coffee_copy_runtime(my_app)` to an existing executable target.

## Embedded SDL is lean by default

Coffee now disables SDL subsystems it does not expose. Windowing, keyboard,
mouse, events, clipboard, filesystem paths, OpenGL, and Vulkan surfaces remain
available. If application code includes `<coffee/native/sdl.h>` and directly
uses SDL audio, gamepads, cameras, haptics, sensors, dialogs, tray, SDL_Render,
notifications, process launching, OpenGL ES, or SDL_GPU, configure with:

```powershell
-DCOFFEE_LEAN_SDL=OFF
```

The standalone wrapper provides the equivalent `-FullSDL` switch.

## Optional application layer

Existing loops need no changes. New projects may derive from
`coffee::Application` and override lifecycle hooks. Fixed updates are disabled
until `application_config::fixed_update_seconds` is positive, and Escape does
not close the application unless `close_on_escape` is enabled.

## Renderer counters

`Renderer::begin_frame()` resets draw statistics. The `Application` loop calls
it automatically; direct loops should call it once at the beginning of a frame
when they use `Renderer::statistics()`.
