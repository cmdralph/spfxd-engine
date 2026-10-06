#pragma once

#include <coffee/api.h>
#include <coffee/math.h>
#include <coffee/shader_preprocessor.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <span>

namespace coffee {

struct shader_sources {
    std::string vertex;
    std::string fragment;
    std::string geometry;
};

class COFFEE_API Shader final {
public:
    Shader(std::string_view vertex_source, std::string_view fragment_source);
    explicit Shader(const shader_sources& sources);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    [[nodiscard]] static Shader from_files(
        const std::filesystem::path& vertex_path,
        const std::filesystem::path& fragment_path,
        const std::filesystem::path& geometry_path = {},
        const ShaderPreprocessor::define_map& defines = {});

    void bind() const noexcept;
    static void unbind() noexcept;

    void set_int(std::string_view name, int value);
    void set_int_array(std::string_view name, std::span<const int> values);
    void set_float(std::string_view name, float value);
    void set_vec2(std::string_view name, vec2 value);
    void set_vec3(std::string_view name, vec3 value);
    void set_vec4(std::string_view name, vec4 value);
    void set_mat4(std::string_view name, const mat4& value);

    [[nodiscard]] std::uint32_t native_handle() const noexcept;
    [[nodiscard]] bool has_uniform(std::string_view name);

private:
    [[nodiscard]] int uniform_location(std::string_view name);
    std::uint32_t handle_ = 0;
    std::unordered_map<std::string, int> uniform_locations_;
};

class COFFEE_API ShaderLibrary final {
public:
    void add(std::string name, std::shared_ptr<Shader> shader);
    [[nodiscard]] std::shared_ptr<Shader> find(std::string_view name) const;
    [[nodiscard]] std::shared_ptr<Shader> require(std::string_view name) const;
    bool remove(std::string_view name);
    void clear() noexcept;

private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
};

} // namespace coffee
