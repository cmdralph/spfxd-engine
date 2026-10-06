#pragma once

#include <coffee/api.h>

#include <stdexcept>
#include <string>

namespace coffee {

enum class error_code {
    unknown,
    invalid_argument,
    platform_initialization,
    window_creation,
    graphics_initialization,
    shader_compilation,
    shader_linking,
    file_io,
    unsupported_format,
    invalid_operation
};

class COFFEE_API error : public std::runtime_error {
public:
    error(error_code code, const std::string& message)
        : std::runtime_error(message), code_(code) {}

    [[nodiscard]] error_code code() const noexcept { return code_; }

private:
    error_code code_;
};

} // namespace coffee
