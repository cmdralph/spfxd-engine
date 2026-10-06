#pragma once

#include <coffee/api.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace coffee {

class COFFEE_API ShaderPreprocessor final {
public:
    using define_map = std::unordered_map<std::string, std::string>;

    [[nodiscard]] static std::string from_file(
        const std::filesystem::path& path,
        const define_map& defines = {});

    [[nodiscard]] static std::string from_source(
        std::string_view source,
        const std::filesystem::path& include_directory = {},
        const define_map& defines = {});
};

} // namespace coffee
