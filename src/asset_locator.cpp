#include <coffee/asset_locator.h>

#include <coffee/error.h>
#include <coffee/file.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <system_error>
#include <utility>

namespace coffee {

namespace {

[[nodiscard]] std::filesystem::path normalized(std::filesystem::path path) {
    std::error_code code;
    auto absolute = std::filesystem::absolute(std::move(path), code);
    if (code) {
        return {};
    }
    auto canonical = std::filesystem::weakly_canonical(absolute, code);
    return code ? absolute.lexically_normal() : canonical;
}

[[nodiscard]] bool path_exists(const std::filesystem::path& path) noexcept {
    std::error_code code;
    return std::filesystem::exists(path, code) && !code;
}

[[nodiscard]] bool inside_root(const std::filesystem::path& candidate,
                               const std::filesystem::path& root) noexcept {
    const auto relative = candidate.lexically_relative(root);
    if (relative.empty()) {
        return false;
    }
    const auto first = relative.begin();
    return first == relative.end() || *first != "..";
}

[[nodiscard]] std::filesystem::path path_from_utf8(const char* value) {
#if defined(_WIN32)
    std::u8string utf8;
    while (*value != '\0') {
        utf8.push_back(static_cast<char8_t>(static_cast<unsigned char>(*value)));
        ++value;
    }
    return std::filesystem::path(utf8);
#else
    return std::filesystem::path(value);
#endif
}

} // namespace

std::filesystem::path executable_directory() {
    const char* path = SDL_GetBasePath();
    if (path == nullptr || *path == '\0') {
        throw error(error_code::file_io,
                    std::string("Could not determine executable directory: ") + SDL_GetError());
    }
    return normalized(path_from_utf8(path));
}

std::filesystem::path preference_directory(const std::string& organization,
                                           const std::string& application) {
    if (organization.empty() || application.empty()) {
        throw error(error_code::invalid_argument,
                    "Preference paths require non-empty organization and application names.");
    }

    char* path = SDL_GetPrefPath(organization.c_str(), application.c_str());
    if (path == nullptr) {
        throw error(error_code::file_io,
                    std::string("Could not create preference directory: ") + SDL_GetError());
    }
    const auto result = normalized(path_from_utf8(path));
    SDL_free(path);
    return result;
}

struct AssetLocator::impl {
    std::vector<std::filesystem::path> roots;
};

AssetLocator::AssetLocator() : impl_(std::make_unique<impl>()) {
    try {
        add_root(executable_directory());
    } catch (const error&) {
        // A current-directory fallback still keeps the locator useful on
        // platforms that cannot report an executable directory.
    }
    add_root(std::filesystem::current_path());
}

AssetLocator::AssetLocator(std::filesystem::path root)
    : impl_(std::make_unique<impl>()) {
    add_root(std::move(root));
}

AssetLocator::~AssetLocator() = default;

AssetLocator::AssetLocator(const AssetLocator& other)
    : impl_(other.impl_ != nullptr
                ? std::make_unique<impl>(*other.impl_)
                : std::make_unique<impl>()) {}

AssetLocator& AssetLocator::operator=(const AssetLocator& other) {
    if (this != &other) {
        auto replacement = other.impl_ != nullptr
            ? std::make_unique<impl>(*other.impl_)
            : std::make_unique<impl>();
        impl_ = std::move(replacement);
    }
    return *this;
}

AssetLocator::AssetLocator(AssetLocator&& other) noexcept = default;
AssetLocator& AssetLocator::operator=(AssetLocator&& other) noexcept = default;

void AssetLocator::add_root(std::filesystem::path root, bool highest_priority) {
    if (root.empty()) {
        throw error(error_code::invalid_argument, "Asset search root cannot be empty.");
    }
    if (impl_ == nullptr) {
        impl_ = std::make_unique<impl>();
    }
    auto value = normalized(std::move(root));
    if (value.empty()) {
        throw error(error_code::invalid_argument, "Asset search root is invalid.");
    }
    if (std::find(impl_->roots.begin(), impl_->roots.end(), value) != impl_->roots.end()) {
        return;
    }
    if (highest_priority) {
        impl_->roots.insert(impl_->roots.begin(), std::move(value));
    } else {
        impl_->roots.push_back(std::move(value));
    }
}

bool AssetLocator::remove_root(const std::filesystem::path& root) {
    if (root.empty() || impl_ == nullptr) {
        return false;
    }
    const auto value = normalized(root);
    const auto before = impl_->roots.size();
    std::erase(impl_->roots, value);
    return impl_->roots.size() != before;
}

void AssetLocator::clear() noexcept {
    if (impl_ != nullptr) {
        impl_->roots.clear();
    }
}

std::vector<std::filesystem::path> AssetLocator::roots() const {
    return impl_ != nullptr ? impl_->roots : std::vector<std::filesystem::path>{};
}

std::optional<std::filesystem::path> AssetLocator::find(
    const std::filesystem::path& asset) const {
    if (asset.empty()) {
        return std::nullopt;
    }
    if (asset.is_absolute()) {
        const auto candidate = normalized(asset);
        return path_exists(candidate) ? std::optional(candidate) : std::nullopt;
    }
    if (impl_ == nullptr) {
        return std::nullopt;
    }
    for (const auto& root : impl_->roots) {
        const auto candidate = normalized(root / asset);
        if (inside_root(candidate, root) && path_exists(candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}

std::filesystem::path AssetLocator::require(const std::filesystem::path& asset) const {
    if (const auto result = find(asset)) {
        return *result;
    }
    throw error(error_code::file_io, "Asset was not found: " + asset.string());
}

bool AssetLocator::contains(const std::filesystem::path& asset) const {
    return find(asset).has_value();
}

std::string AssetLocator::read_text(const std::filesystem::path& asset) const {
    return read_text_file(require(asset));
}

std::vector<std::byte> AssetLocator::read_binary(
    const std::filesystem::path& asset) const {
    return read_binary_file(require(asset));
}

} // namespace coffee
