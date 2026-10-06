#pragma once

#include <coffee/config.h>

namespace coffee {

inline constexpr int version_major = COFFEE_VERSION_MAJOR;
inline constexpr int version_minor = COFFEE_VERSION_MINOR;
inline constexpr int version_patch = COFFEE_VERSION_PATCH;
inline constexpr const char* version_string = COFFEE_VERSION_STRING;

} // namespace coffee
