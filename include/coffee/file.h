#pragma once

#include <coffee/api.h>

#include <filesystem>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace coffee {

[[nodiscard]] COFFEE_API std::string read_text_file(const std::filesystem::path& path);
[[nodiscard]] COFFEE_API std::vector<std::byte> read_binary_file(const std::filesystem::path& path);
COFFEE_API void write_text_file(const std::filesystem::path& path, std::string_view text);
COFFEE_API void write_binary_file(const std::filesystem::path& path,
                                  std::span<const std::byte> data);

// Creates a directory tree and succeeds when it already exists.
COFFEE_API void ensure_directory(const std::filesystem::path& path);

} // namespace coffee
