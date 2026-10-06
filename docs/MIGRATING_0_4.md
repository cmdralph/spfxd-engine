# Migrating from Coffee 0.3 to 0.4

## Build changes

- `coffee_static` and `COFFEE_BUILD_STATIC` were removed from the default SDK.
- `coffee_shared` became the concrete `coffee` target.
- `coffee::coffee` is the recommended target; `coffee::shared` remains as a
  source-tree compatibility alias.
- SDL3 must now be source/static, because it is embedded in `Coffee.dll`.
- SDL3 headers/libraries are no longer public dependencies or installed files.
- Output directories are `build/.../bin/<Config>` and `lib/<Config>`.
- Debug no longer changes the runtime filename to `Coffeed.dll`; configurations
  live in separate output directories instead.

Use a new generator-specific build directory instead of reusing the old cache:

```powershell
cmake --preset windows-msvc
```

The `windows-msvc` preset now uses standalone `cl.exe` with Ninja. Visual
Studio is optional under the separate `windows-vs2022` preset.

## Vulkan changes

- `COFFEE_ENABLE_VULKAN` became `COFFEE_BUILD_VULKAN_MODULE`.
- Vulkan builds as `CoffeeVulkan.dll` and target `coffee::vulkan`.
- Core OpenGL applications never link the Vulkan loader.
- `COFFEE_HAS_VULKAN_MODULE` is the new capability macro.
- `COFFEE_HAS_VULKAN` remains a 0.3-compatible alias for this release.

## Native API changes

`coffee/native/opengl.h` no longer exposes Coffee's private GLAD header. Use
`coffee::native::opengl_proc_address()` with your preferred loader for raw GL.
`coffee/native/sdl.h` forward-declares `SDL_Window`; normal use needs no SDL
header or import library.

The normal high-level APIs are otherwise source-compatible.
