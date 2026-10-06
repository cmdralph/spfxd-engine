#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace coffee {

// A deliberately small, ownership-friendly cache. The cache does not keep
// resources alive after the application releases its final shared_ptr.
template<typename Resource>
class ResourceCache final {
public:
    template<typename Loader>
    std::shared_ptr<Resource> get_or_load(std::string key, Loader&& loader) {
        if (const auto found = resources_.find(key); found != resources_.end()) {
            if (auto existing = found->second.lock()) return existing;
        }
        auto loaded = std::forward<Loader>(loader)();
        if (loaded) resources_[std::move(key)] = loaded;
        return loaded;
    }

    [[nodiscard]] std::shared_ptr<Resource> find(std::string_view key) const {
        const auto found = resources_.find(std::string(key));
        return found != resources_.end() ? found->second.lock() : nullptr;
    }

    void remove(std::string_view key) { resources_.erase(std::string(key)); }
    void clear() noexcept { resources_.clear(); }

    void remove_expired() {
        for (auto iterator = resources_.begin(); iterator != resources_.end();) {
            if (iterator->second.expired()) iterator = resources_.erase(iterator);
            else ++iterator;
        }
    }

    [[nodiscard]] std::size_t size() const noexcept { return resources_.size(); }

private:
    std::unordered_map<std::string, std::weak_ptr<Resource>> resources_;
};

} // namespace coffee

