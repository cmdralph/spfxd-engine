# Third-party notices

Coffee's binary distribution may contain code from these projects. Keep each
dependency's original license file with source distributions and review it
before redistributing a compiled SDK.

## SDL3

- Project: Simple DirectMedia Layer 3
- Website: https://www.libsdl.org/
- License: zlib license
- Role: windowing, input, platform integration, and native BMP decoding
- Distribution: linked statically into `Coffee.dll`

The SDL source package contains its complete license in `LICENSE.txt`.

## GLAD

- Project: GLAD OpenGL loader generator
- Website: https://github.com/Dav1dde/glad
- Version: 2.0.8 generator output
- License: generated loader under WTFPL or CC0-1.0; Khronos platform header under Apache-2.0
- Role: OpenGL 3.3 Core function loading
- Distribution: generated loader implementation compiled into `Coffee.dll`

Retain the license comments and files shipped with the exact generated GLAD
output used by your build.

## stb_image

- Project: stb single-file public-domain/MIT libraries
- Website: https://github.com/nothings/stb
- License: dual-licensed public domain or MIT
- Role: optional PNG, JPEG, TGA, PSD, GIF, HDR, and PIC decoding
- Distribution: implementation compiled into `Coffee.dll` when enabled

The full MIT text is included at the end of the upstream `stb_image.h` file.

## Vulkan

- Project: Vulkan SDK and loader
- Website: https://vulkan.lunarg.com/
- Role: optional `CoffeeVulkan.dll` backend
- Distribution: not embedded in `Coffee.dll`; provided by the system/SDK

Coffee's own license is in `LICENSE.txt`.
