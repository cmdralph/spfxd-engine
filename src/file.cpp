#include <coffee/file.h>
#include <coffee/error.h>

#include <fstream>
#include <iterator>
#include <limits>
#include <system_error>

namespace coffee {

namespace {

[[nodiscard]] std::streamsize checked_stream_size(std::size_t size,
                                                  const std::filesystem::path& path) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        throw error(error_code::file_io, "File is too large for one operation: " + path.string());
    }
    return static_cast<std::streamsize>(size);
}

} // namespace

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw error(error_code::file_io, "Could not open text file: " + path.string());
    }
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw error(error_code::file_io, "Could not open binary file: " + path.string());
    }
    const auto end = stream.tellg();
    if (end < 0) {
        throw error(error_code::file_io, "Could not determine file size: " + path.string());
    }
    std::vector<std::byte> data(static_cast<std::size_t>(end));
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(data.data()), checked_stream_size(data.size(), path));
    if (!stream && !data.empty()) {
        throw error(error_code::file_io, "Could not read complete file: " + path.string());
    }
    return data;
}

void write_text_file(const std::filesystem::path& path, std::string_view text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw error(error_code::file_io, "Could not open text file for writing: " + path.string());
    }
    stream.write(text.data(), checked_stream_size(text.size(), path));
    if (!stream) {
        throw error(error_code::file_io, "Could not write complete text file: " + path.string());
    }
}

void write_binary_file(const std::filesystem::path& path,
                       std::span<const std::byte> data) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw error(error_code::file_io, "Could not open binary file for writing: " + path.string());
    }
    stream.write(reinterpret_cast<const char*>(data.data()),
                 checked_stream_size(data.size(), path));
    if (!stream) {
        throw error(error_code::file_io, "Could not write complete binary file: " + path.string());
    }
}

void ensure_directory(const std::filesystem::path& path) {
    if (path.empty()) {
        throw error(error_code::invalid_argument, "Directory path cannot be empty.");
    }
    std::error_code code;
    std::filesystem::create_directories(path, code);
    if (code || !std::filesystem::is_directory(path)) {
        throw error(error_code::file_io, "Could not create directory: " + path.string());
    }
}

} // namespace coffee
