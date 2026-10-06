#include <coffee/shader.h>
#include <coffee/error.h>

#include <glad/gl.h>

#include <algorithm>
#include <utility>
#include <vector>

namespace coffee {

namespace {

std::uint32_t compile_stage(GLenum type, std::string_view source, const char* stage_name) {
    const std::uint32_t shader = glCreateShader(type);
    const char* source_data = source.data();
    const GLint source_length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &source_data, &source_length);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) return shader;

    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> log(static_cast<std::size_t>(std::max(log_length, 1)));
    glGetShaderInfoLog(shader, log_length, nullptr, log.data());
    glDeleteShader(shader);
    throw error(error_code::shader_compilation,
                std::string(stage_name) + " shader compilation failed:\n" + log.data());
}

} // namespace

Shader::Shader(std::string_view vertex_source, std::string_view fragment_source)
    : Shader(shader_sources{std::string(vertex_source), std::string(fragment_source), {}}) {}

Shader::Shader(const shader_sources& sources) {
    const std::uint32_t vertex = compile_stage(GL_VERTEX_SHADER, sources.vertex, "Vertex");
    std::uint32_t fragment = 0;
    std::uint32_t geometry = 0;
    try {
        fragment = compile_stage(GL_FRAGMENT_SHADER, sources.fragment, "Fragment");
        if (!sources.geometry.empty()) geometry = compile_stage(GL_GEOMETRY_SHADER, sources.geometry, "Geometry");
    } catch (...) {
        glDeleteShader(vertex);
        if (fragment != 0) glDeleteShader(fragment);
        throw;
    }

    handle_ = glCreateProgram();
    glAttachShader(handle_, vertex);
    glAttachShader(handle_, fragment);
    if (geometry != 0) glAttachShader(handle_, geometry);
    glLinkProgram(handle_);
    glDetachShader(handle_, vertex);
    glDetachShader(handle_, fragment);
    if (geometry != 0) glDetachShader(handle_, geometry);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (geometry != 0) glDeleteShader(geometry);

    GLint linked = GL_FALSE;
    glGetProgramiv(handle_, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) return;

    GLint log_length = 0;
    glGetProgramiv(handle_, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> log(static_cast<std::size_t>(std::max(log_length, 1)));
    glGetProgramInfoLog(handle_, log_length, nullptr, log.data());
    glDeleteProgram(handle_);
    handle_ = 0;
    throw error(error_code::shader_linking, std::string("Shader program link failed:\n") + log.data());
}

Shader::~Shader() { if (handle_ != 0) glDeleteProgram(handle_); }
Shader::Shader(Shader&& other) noexcept
    : handle_(std::exchange(other.handle_, 0)), uniform_locations_(std::move(other.uniform_locations_)) {}
Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (handle_ != 0) glDeleteProgram(handle_);
        handle_ = std::exchange(other.handle_, 0);
        uniform_locations_ = std::move(other.uniform_locations_);
    }
    return *this;
}

Shader Shader::from_files(const std::filesystem::path& vertex_path,
                          const std::filesystem::path& fragment_path,
                          const std::filesystem::path& geometry_path,
                          const ShaderPreprocessor::define_map& defines) {
    return Shader(shader_sources{
        ShaderPreprocessor::from_file(vertex_path, defines),
        ShaderPreprocessor::from_file(fragment_path, defines),
        geometry_path.empty()
            ? std::string{}
            : ShaderPreprocessor::from_file(geometry_path, defines)
    });
}
void Shader::bind() const noexcept { glUseProgram(handle_); }
void Shader::unbind() noexcept { glUseProgram(0); }
void Shader::set_int(std::string_view name, int value) { bind(); glUniform1i(uniform_location(name), value); }
void Shader::set_int_array(std::string_view name, std::span<const int> values) {
    bind();
    glUniform1iv(uniform_location(name), static_cast<GLsizei>(values.size()), values.data());
}
void Shader::set_float(std::string_view name, float value) { bind(); glUniform1f(uniform_location(name), value); }
void Shader::set_vec2(std::string_view name, vec2 value) { bind(); glUniform2f(uniform_location(name), value.x, value.y); }
void Shader::set_vec3(std::string_view name, vec3 value) { bind(); glUniform3f(uniform_location(name), value.x, value.y, value.z); }
void Shader::set_vec4(std::string_view name, vec4 value) { bind(); glUniform4f(uniform_location(name), value.x, value.y, value.z, value.w); }
void Shader::set_mat4(std::string_view name, const mat4& value) {
    bind();
    glUniformMatrix4fv(uniform_location(name), 1, GL_FALSE, value.data());
}
std::uint32_t Shader::native_handle() const noexcept { return handle_; }
bool Shader::has_uniform(std::string_view name) { return uniform_location(name) >= 0; }
int Shader::uniform_location(std::string_view name) {
    const std::string key(name);
    if (const auto found = uniform_locations_.find(key); found != uniform_locations_.end()) return found->second;
    const int location = glGetUniformLocation(handle_, key.c_str());
    uniform_locations_.emplace(key, location);
    return location;
}

void ShaderLibrary::add(std::string name, std::shared_ptr<Shader> shader) {
    if (name.empty() || !shader) throw error(error_code::invalid_argument, "Shader library entries require a name and shader.");
    shaders_.insert_or_assign(std::move(name), std::move(shader));
}

std::shared_ptr<Shader> ShaderLibrary::find(std::string_view name) const {
    const auto found = shaders_.find(std::string(name));
    return found != shaders_.end() ? found->second : nullptr;
}

std::shared_ptr<Shader> ShaderLibrary::require(std::string_view name) const {
    auto result = find(name);
    if (!result) throw error(error_code::invalid_argument, "Shader was not found in the library: " + std::string(name));
    return result;
}

bool ShaderLibrary::remove(std::string_view name) { return shaders_.erase(std::string(name)) != 0; }
void ShaderLibrary::clear() noexcept { shaders_.clear(); }

} // namespace coffee
