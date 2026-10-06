# Coffee-GE
### Temporary branch to merge later.

Coffee Graphics Engine is a compact C++ graphics and game-foundation library for games,
desktop tools, interfaces, and visualization applications. OpenGL 3.3 Core is
the built-in renderer; Vulkan is an optional companion module.

### Disclaimer!
Most of the code seen in this repo is written by AI, but tested and reviewed by myself.
It's definitely nowhere near production ready and I haven't fully tested it but 
I like trying to see what I can achieve with these new AIs.

```cpp
#include <coffee/coffee.h>

class game final : public coffee::Application {
public:
    game() : Application([] {
        coffee::application_config config;
        config.window.title = "My application";
        config.close_on_escape = true;
        return config;
    }()) {}

private:
    void on_update(double delta_seconds) override {
        // Update game or interface state.
    }

    void on_render(double interpolation_alpha) override {
        // Submit drawing commands. Coffee clears and presents automatically.
    }
};

int main() {
    return coffee::run_application<game>();
}
```

The direct `Window` loop remains available when an application wants complete
control. `Application` is a convenience layer, not a mandatory framework.

## Distribution model

The normal build compiles the engine once and produces one runtime library:

```text
Coffee.dll       engine + SDL3 + GLAD + optional stb_image
Coffee.lib       MSVC import library
include/coffee/  public C++ headers
```

SDL3 must be supplied as source or as an installed static CMake target. Its
implementation is then linked into `Coffee.dll`; applications do not ship
`SDL3.dll` and do not link SDL, GLAD, or stb_image themselves. Coffee exports
only its documented API instead of every third-party symbol.

Vulkan is deliberately separate:

```text
CoffeeVulkan.dll  optional context/device/swapchain module
```

This keeps the common OpenGL download and runtime lean. See
[`docs/VULKAN.md`](docs/VULKAN.md) for its current scope.

## Build on Windows

First put these dependencies in place:

```text
deps/SDL3/                 SDL3 source tree, including CMakeLists.txt
deps/glad/                 bundled GLAD 2 OpenGL 3.3 Core loader
deps/stb/stb_image.h       optional
```

Visual Studio is not required. Initialize your standalone MSVC and Windows SDK
environment so `cl`, `link`, `rc`, and `mt` are available, then use Ninja:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-release
ctest --preset windows-release
cmake --install build/windows-msvc-release
```

The install step creates the redistributable SDK in `dist/Coffee`. The runtime
DLL and examples are built under `build/windows-msvc-release/bin`.

Or let the build helper verify the standalone toolchain and choose Ninja/NMake:

```powershell
.\scripts\build-msvc.ps1 -Configuration Release -Install -Package
```

## Use the installed SDK

```cmake
cmake_minimum_required(VERSION 3.23)
project(MyCoffeeApp LANGUAGES CXX)

find_package(Coffee 0.5 CONFIG REQUIRED)
coffee_add_executable(my_app src/main.cpp)
```

Configure with Coffee's install prefix if needed:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/path/to/Coffee/dist/Coffee"
```

`coffee_add_executable()` links Coffee and copies `Coffee.dll` beside the
executable after every build. Existing targets can use
`coffee_copy_runtime(my_app)` instead. A C++ DLL is not a universal ABI:
distribute separate SDK builds per architecture/toolchain and keep the MSVC
runtime family compatible between Coffee and its application.

## Included systems

- Optional application lifecycle with update, fixed-update, render, and event hooks
- RAII windows, input, richer events, clipboard, high-DPI handling, clocks, and fixed timesteps
- OpenGL shaders, buffers, vertex arrays, textures, framebuffers, render state,
  meshes, and batched 2D sprites
- Vectors, matrices, quaternions, transforms, and 2D/3D cameras
- CPU images with BMP plus optional stb_image formats
- Recursive shader includes/defines and a shader resource library
- Lightweight 2D rigid bodies, contacts, sensors, impulses, and raycasts
- Structured logging with a replaceable callback and zero mandatory formatting dependency
- Portable executable/preference paths, asset lookup, file reads/writes, and weak resource caches
- Per-frame renderer statistics, driver information, and instanced indexed drawing
- Optional Vulkan window/device/swapchain frame lifecycle

Coffee stays intentionally focused. Its built-in physics is a small 2D solver,
not a full 3D physics replacement, and the Vulkan module does not pretend that
OpenGL objects are portable Vulkan resources.

# Docs
Start with [`docs/GETTING_STARTED.md`](docs/GETTING_STARTED.md), then read
[`docs/BUILDING.md`](docs/BUILDING.md),
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).
