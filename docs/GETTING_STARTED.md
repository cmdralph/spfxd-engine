# Getting started with Coffee 0.5

## 1. Install or unpack the SDK

An installed Coffee SDK contains:

```text
Coffee/
├── bin/Coffee.dll
├── include/coffee/...
├── lib/Coffee.lib
└── lib/cmake/Coffee/...
```

## 2. Create a tiny application

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.23)
project(hello_coffee LANGUAGES CXX)

find_package(Coffee 0.5 CONFIG REQUIRED)
coffee_add_executable(hello_coffee main.cpp)
```

`main.cpp`:

```cpp
#include <coffee/coffee.h>

class hello final : public coffee::Application {
public:
    hello() : Application([] {
        coffee::application_config config;
        config.window.title = "Hello, Coffee";
        config.window.width = 1000;
        config.window.height = 650;
        config.close_on_escape = true;
        config.clear_color = {0.04f, 0.07f, 0.11f, 1.0f};
        return config;
    }()) {}

private:
    void on_update(double delta_seconds) override {
        // Advance animations, interface state, or gameplay.
    }

    void on_render(double interpolation_alpha) override {
        // Use renderer(), SpriteBatch, Mesh, Shader, and other Coffee objects.
    }
};

int main() {
    return coffee::run_application<hello>();
}
```

Configure and build, pointing CMake at the SDK:

```powershell
cmake -S . -B build -G Ninja `
    -DCMAKE_PREFIX_PATH="C:/Libraries/Coffee"
cmake --build build
.\build\hello_coffee.exe
```

`coffee_add_executable()` links the import library and automatically stages
`Coffee.dll` beside the program. Use ordinary `add_executable()` plus
`coffee_copy_runtime(target)` if you need custom target setup.

## Application lifecycle

Coffee calls hooks in this order:

1. `on_start()` once, after the OpenGL context exists.
2. Poll events and call `on_event()` for each event.
3. Call `on_fixed_update()` zero or more times when fixed updates are enabled.
4. Call `on_update()` once per visible frame.
5. Clear, call `on_render()`, then present the OpenGL backbuffer.
6. `on_shutdown()` once after a normal exit or runtime exception.

Set `fixed_update_seconds` to a positive duration, commonly `1.0 / 60.0`, to
enable fixed updates. Leave it at zero for tools and interfaces that only need
frame updates.

## Assets and writable data

```cpp
coffee::AssetLocator assets;
assets.add_root("assets", true);

const auto shader_text = assets.read_text("shaders/basic.vert");
const auto save_directory = coffee::preference_directory("My Studio", "My Game");
coffee::ensure_directory(save_directory);
coffee::write_text_file(save_directory / "settings.ini", "vsync=true\n");
```

The default asset locator searches beside the executable and then in the
current working directory. Preference paths use the correct writable location
for each operating system.

## Diagnostics

Coffee logs compact messages to stderr by default:

```cpp
coffee::set_log_level(coffee::log_level::debug);
coffee::log_info("assets", "Loading interface theme");
```

`set_log_callback()` can route records into an editor console, file logger, or
existing application logging system. The callback is synchronous, may run on
any thread, and must not retain the record's string views.

## Shader includes and variants

`Shader::from_files()` automatically runs Coffee's shader preprocessor. Quoted
`#include` directives resolve relative to the including file, `#pragma once`
works across a shader include graph, and compile-time variants can pass defines:

```cpp
coffee::ShaderPreprocessor::define_map defines{
    {"MAX_LIGHTS", "8"},
    {"USE_NORMAL_MAP", "1"}
};

auto shader = coffee::Shader::from_files(
    assets.require("shaders/model.vert"),
    assets.require("shaders/model.frag"),
    {},
    defines);
```

## Direct-loop alternative

`Application` is optional. Lower-level code can still own `coffee::Window`,
call `poll_events()`, update state, render, and call `present()` explicitly.
This is appropriate for multiple windows, custom schedulers, and external GUI
framework integration.
