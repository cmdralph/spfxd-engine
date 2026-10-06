# Building and distributing Coffee

Coffee uses ordinary CMake targets and does not require the Visual Studio IDE.
The recommended Windows path is standalone MSVC with the Ninja generator.
NMake and Visual Studio generators remain supported alternatives.

## Required tools

- CMake 3.23 or newer
- Standalone MSVC with `cl.exe` and `link.exe`
- Windows SDK with `rc.exe`, `mt.exe`, headers, and libraries
- Ninja (recommended) or NMake
- SDL 3.2+ source or an installed `SDL3::SDL3-static` target
- Bundled GLAD 2 files from the Coffee source package
- Optional `stb_image.h`
- Optional Vulkan SDK for `CoffeeVulkan.dll`

Your shell must expose the toolchain before configuring. These commands should
all resolve to the intended standalone installation:

```powershell
Get-Command cmake, cl, link, rc, mt, ninja
$env:INCLUDE
$env:LIB
```

Coffee intentionally does not hardcode MSVC or Windows SDK installation paths.
Portable toolchain layouts differ, and embedding one machine's paths in an SDK
would make the build less reliable. Initialize the environment using the setup
script supplied with your standalone toolchain, or set `PATH`, `INCLUDE`,
`LIB`, and `LIBPATH` before configuring.

## Dependency layout

```text
deps/
├── SDL3/                    complete SDL3 source tree
│   ├── CMakeLists.txt
│   ├── include/SDL3/...
│   └── src/...
├── glad/
│   ├── include/glad/gl.h
│   ├── include/KHR/khrplatform.h
│   └── src/gl.c
└── stb/
    └── stb_image.h          optional

```

GLAD is already part of Coffee and requires no separate download or generation
step. If its three files are missing, the source package was copied or
extracted incompletely.

The `SDL3.lib` shipped beside `SDL3.dll` is an import library and cannot be
embedded. Use SDL source or an installed package providing
`SDL3::SDL3-static`.

## Simplest standalone MSVC workflow

The wrapper verifies the compiler, linker, Windows SDK tools, environment, and
build generator before calling CMake:

```powershell
.\scripts\build-msvc.ps1 -Configuration Release -Install -Package
```

It prefers Ninja and falls back to NMake. Other useful forms:

```powershell
# Debug build
.\scripts\build-msvc.ps1 -Configuration Debug

# Force NMake
.\scripts\build-msvc.ps1 -Generator NMake -Configuration Release

# Build the optional Vulkan module
.\scripts\build-msvc.ps1 -Configuration Release -Vulkan -Install

# Configure/build only
.\scripts\build-msvc.ps1 -Configuration Release -SkipTests

# Retain every SDL subsystem for native SDL integrations
.\scripts\build-msvc.ps1 -Configuration Release -FullSDL

# Discard configure metadata and detect the toolchain again
.\scripts\build-msvc.ps1 -Configuration Release -Fresh
```

Each generator/configuration receives a separate build directory, avoiding
cache conflicts such as reusing a Visual Studio build tree with Ninja.

The wrapper converts standalone compiler and Windows SDK paths to CMake's
forward-slash form. This matters when an installation path contains spaces or
backslashes, such as `C:\msvc\Windows Kits\...`. With CMake 3.24 or newer it
uses a fresh configure pass until the first configure succeeds. Later builds
reuse the healthy cache for speed; `-Fresh` explicitly resets it when needed.

### Invalid character escape in `CMakeRCCompiler.cmake`

Coffee 0.5.1 fixes this wrapper issue. It occurred when a raw `rc.exe` path was
written into generated CMake code and a sequence such as `\m` was interpreted
as an escape. Update `scripts/build-msvc.ps1` and run the same command again;
manual deletion of the build directory is unnecessary with CMake 3.24+.

### Bundled GLAD loader is incomplete

Coffee 0.5.2 includes its OpenGL 3.3 Core loader directly under `deps/glad`.
If CMake reports that it is incomplete, re-extract the complete Coffee source
archive rather than downloading a different GLAD version. Coffee's internal
code targets the bundled GLAD 2 API.

## Standalone MSVC presets

The recommended preset uses single-configuration Ninja and `cl.exe`:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-release
ctest --preset windows-release
cmake --install build/windows-msvc-release
```

Debug:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

NMake fallback:

```powershell
cmake --preset windows-msvc-nmake
cmake --build --preset windows-nmake-release
ctest --preset windows-nmake-release
cmake --install build/windows-msvc-nmake-release
```

The optional Visual Studio generator still exists for contributors who have it:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-vs2022-release
ctest --preset windows-vs2022-release
cmake --install build/windows-vs2022 --config Release
```

## Manual generator-neutral commands

Presets are convenience only. This performs the same standalone Ninja build:

```powershell
cmake -S . -B build\manual-msvc-release -G Ninja `
    -DCMAKE_C_COMPILER=cl `
    -DCMAKE_CXX_COMPILER=cl `
    -DCMAKE_BUILD_TYPE=Release `
    -DCOFFEE_BUILD_EXAMPLES=ON `
    -DCOFFEE_BUILD_TESTS=ON

cmake --build build\manual-msvc-release --parallel
ctest --test-dir build\manual-msvc-release --output-on-failure
cmake --install build\manual-msvc-release --prefix dist\Coffee
```

For Ninja and NMake, configuration is chosen when CMake configures, so do not
append `--config Release`. Visual Studio is multi-configuration and does use
`--config Release`.

## Outputs

Single-configuration Ninja/NMake:

```text
build/windows-msvc-release/
├── bin/
│   ├── Coffee.dll
│   └── coffee_*.exe
└── lib/
    └── Coffee.lib
```

Installed SDK:

```text
dist/Coffee/
├── bin/Coffee.dll
├── include/coffee/...
├── lib/Coffee.lib
├── lib/cmake/Coffee/...
└── share/Coffee/...
```

No `SDL3.dll` is installed. Verify the runtime dependencies with:

```powershell
dumpbin /DEPENDENTS dist\Coffee\bin\Coffee.dll
```

## Build the Vulkan module

With the Vulkan SDK environment initialized:

```powershell
cmake --preset windows-msvc-vulkan
cmake --build --preset windows-vulkan-release
ctest --preset windows-vulkan-release
cmake --install build/windows-msvc-vulkan-release
```

This adds `CoffeeVulkan.dll` and `CoffeeVulkan.lib`; the OpenGL core remains
`Coffee.dll`.

## Options

| Option | Default | Meaning |
| --- | ---: | --- |
| `COFFEE_BUILD_VULKAN_MODULE` | `OFF` | Build separate `CoffeeVulkan.dll` |
| `COFFEE_ENABLE_STB_IMAGE` | `ON` | Embed stb_image when its header exists |
| `COFFEE_ENABLE_LTO` | `ON` | Enable release LTO when supported |
| `COFFEE_LEAN_SDL` | `ON` | Exclude SDL systems Coffee does not expose |
| `COFFEE_BUILD_EXAMPLES` | top-level `ON` | Build examples |
| `COFFEE_BUILD_TESTS` | top-level `ON` | Build CTest targets |
| `COFFEE_INSTALL` | `ON` | Generate SDK install rules |
| `COFFEE_WARNINGS_AS_ERRORS` | `OFF` | Make Coffee warnings fatal |

## Consume the installed package

```cmake
find_package(Coffee 0.5 CONFIG REQUIRED)
coffee_add_executable(my_app src/main.cpp)
```

Vulkan component:

```cmake
find_package(Coffee 0.5 CONFIG REQUIRED COMPONENTS Vulkan)
target_link_libraries(my_app PRIVATE coffee::coffee coffee::vulkan)
coffee_copy_runtime(my_app)
```

For a non-CMake project, add `dist/Coffee/include` to includes,
`dist/Coffee/lib` to linker directories, link `Coffee.lib`, and copy
`dist/Coffee/bin/Coffee.dll` beside the executable.

## Package the SDK

For the standalone Ninja build:

```powershell
.\scripts\build-msvc.ps1 -Configuration Release -Package
```

Release separate SDKs per CPU architecture and materially different compiler
runtime families. Coffee exposes a C++20 ABI, not a compiler-independent C ABI.
