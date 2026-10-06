#pragma once

// Coffee has a deliberately small exported ABI. Third-party implementation
// symbols (SDL, GLAD, stb_image) remain private to Coffee.dll.
#if defined(_WIN32)
    #if defined(COFFEE_STATIC)
        #define COFFEE_API
    #elif defined(COFFEE_BUILDING_CORE)
        #define COFFEE_API __declspec(dllexport)
    #else
        #define COFFEE_API __declspec(dllimport)
    #endif

    #if defined(COFFEE_BUILDING_VULKAN)
        #define COFFEE_VULKAN_API __declspec(dllexport)
    #else
        #define COFFEE_VULKAN_API __declspec(dllimport)
    #endif
#else
    #define COFFEE_API __attribute__((visibility("default")))
    #define COFFEE_VULKAN_API __attribute__((visibility("default")))
#endif
