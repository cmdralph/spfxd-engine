#include <coffee/log.h>

#include <atomic>
#include <cstdio>
#include <mutex>

namespace coffee {

namespace {

std::atomic<log_level> minimum_level{log_level::info};
std::mutex callback_mutex;
std::mutex output_mutex;
log_callback active_callback = nullptr;
void* active_user_data = nullptr;

[[nodiscard]] const char* level_name(log_level level) noexcept {
    switch (level) {
        case log_level::trace: return "trace";
        case log_level::debug: return "debug";
        case log_level::info: return "info";
        case log_level::warning: return "warning";
        case log_level::error: return "error";
        case log_level::critical: return "critical";
        case log_level::off: return "off";
    }
    return "unknown";
}

void default_log(const log_record& record) noexcept {
    std::scoped_lock lock(output_mutex);
    std::fputs("[Coffee][", stderr);
    std::fputs(level_name(record.level), stderr);
    std::fputc(']', stderr);
    if (!record.category.empty()) {
        std::fputc('[', stderr);
        std::fwrite(record.category.data(), 1, record.category.size(), stderr);
        std::fputc(']', stderr);
    }
    std::fputc(' ', stderr);
    std::fwrite(record.message.data(), 1, record.message.size(), stderr);
    std::fputc('\n', stderr);
    std::fflush(stderr);
}

} // namespace

void set_log_level(log_level level) noexcept {
    minimum_level.store(level, std::memory_order_relaxed);
}

log_level current_log_level() noexcept {
    return minimum_level.load(std::memory_order_relaxed);
}

void set_log_callback(log_callback callback, void* user_data) noexcept {
    std::scoped_lock lock(callback_mutex);
    active_callback = callback;
    active_user_data = callback != nullptr ? user_data : nullptr;
}

void write_log(log_level level, std::string_view category,
               std::string_view message) noexcept {
    const auto threshold = minimum_level.load(std::memory_order_relaxed);
    if (level == log_level::off || threshold == log_level::off ||
        static_cast<int>(level) < static_cast<int>(threshold)) {
        return;
    }

    log_callback callback = nullptr;
    void* user_data = nullptr;
    {
        std::scoped_lock lock(callback_mutex);
        callback = active_callback;
        user_data = active_user_data;
    }

    const log_record record{level, category, message};
    if (callback != nullptr) {
        callback(record, user_data);
    } else {
        default_log(record);
    }
}

} // namespace coffee
