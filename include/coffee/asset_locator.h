#pragma once

#include <coffee/api.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace coffee {

// Directory containing the running executable or application bundle resources.
[[nodiscard]] COFFEE_API std::filesystem::path executable_directory();

// A writable, per-user directory suitable for settings, saves, and caches.
// The directory is created by the platform layer when necessary.
[[nodiscard]] COFFEE_API std::filesystem::path preference_directory(
    const std::string& organization,
    const std::string& application);

class COFFEE_API AssetLocator final {
public:
    // Searches the executable directory first, followed by the current directory.
    AssetLocator();

    // Creates a locator with exactly one initial search root.
    explicit AssetLocator(std::filesystem::path root);
    ~AssetLocator();

    AssetLocator(const AssetLocator& other);
    AssetLocator& operator=(const AssetLocator& other);
    AssetLocator(AssetLocator&& other) noexcept;
    AssetLocator& operator=(AssetLocator&& other) noexcept;

    void add_root(std::filesystem::path root, bool highest_priority = false);
    bool remove_root(const std::filesystem::path& root);
    void clear() noexcept;

    [[nodiscard]] std::vector<std::filesystem::path> roots() const;
    [[nodiscard]] std::optional<std::filesystem::path> find(
        const std::filesystem::path& asset) const;
    [[nodiscard]] std::filesystem::path require(
        const std::filesystem::path& asset) const;
    [[nodiscard]] bool contains(const std::filesystem::path& asset) const;

    [[nodiscard]] std::string read_text(const std::filesystem::path& asset) const;
    [[nodiscard]] std::vector<std::byte> read_binary(
        const std::filesystem::path& asset) const;

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace coffee
