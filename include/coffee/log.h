#pragma once

#include <coffee/api.h>

#include <string_view>

namespace coffee {

enum class log_level {
    trace,
    debug,
    info,
    warning,
    error,
    critical,
    off
};

struct log_record {
    log_level level = log_level::info;
    std::string_view category{};
    std::string_view message{};
};

// The record's string views remain valid only for the duration of the call.
// Callbacks must not throw and may be invoked from any thread.
using log_callback = void (*)(const log_record& record, void* user_data) noexcept;

COFFEE_API void set_log_level(log_level level) noexcept;
[[nodiscard]] COFFEE_API log_level current_log_level() noexcept;

// Passing nullptr restores Coffee's compact stderr logger.
COFFEE_API void set_log_callback(log_callback callback, void* user_data = nullptr) noexcept;
COFFEE_API void write_log(log_level level, std::string_view category,
                          std::string_view message) noexcept;

inline void log_trace(std::string_view category, std::string_view message) noexcept {
    write_log(log_level::trace, category, message);
}

inline void log_debug(std::string_view category, std::string_view message) noexcept {
    write_log(log_level::debug, category, message);
}

inline void log_info(std::string_view category, std::string_view message) noexcept {
    write_log(log_level::info, category, message);
}

inline void log_warning(std::string_view category, std::string_view message) noexcept {
    write_log(log_level::warning, category, message);
}

inline void log_error(std::string_view category, std::string_view message) noexcept {
    write_log(log_level::error, category, message);
}

inline void log_critical(std::string_view category, std::string_view message) noexcept {
    write_log(log_level::critical, category, message);
}

} // namespace coffee
