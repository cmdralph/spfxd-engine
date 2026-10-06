# Dependency setup

Coffee bundles its small generated OpenGL loader. SDL3 and optional image or
Vulkan dependencies remain separate so their versions can be updated without
silently replacing a developer's selected platform SDK.

## SDL3 (bundled)

```text
deps/SDL3/CMakeLists.txt
deps/SDL3/include/SDL3/...
deps/SDL3/src/...
```

Coffee forces SDL's static target on and shared target off, then links it into
`Coffee.dll`. The official prebuilt `SDL3.lib` beside `SDL3.dll` is an import
library and is not enough for this mode.

An installed CMake package that provides `SDL3::SDL3-static` also works.

## GLAD 2 (bundled)

Coffee ships a reproducible GLAD 2 loader generated for OpenGL 3.3 Core with
no extensions:

```text
deps/glad/include/glad/gl.h
deps/glad/include/KHR/khrplatform.h
deps/glad/src/gl.c
```

Applications do not install, include, or link GLAD themselves. If these files
are missing, re-extract the complete Coffee source package.

## stb_image (optional)

Place the official single header at:

```text
deps/stb/stb_image.h
```

It is compiled into `Coffee.dll`. Without it, Coffee keeps SDL-backed BMP
loading and reports `COFFEE_HAS_STB_IMAGE` as zero.

## Vulkan (optional)

Install the Vulkan SDK and configure with `COFFEE_BUILD_VULKAN_MODULE=ON`.
Vulkan is discovered through CMake and is not copied into this folder.
