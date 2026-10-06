cmake_minimum_required(VERSION 3.23)

project(Coffee VERSION 0.5.2 LANGUAGES C CXX)

option(COFFEE_BUILD_EXAMPLES "Build Coffee examples" ${PROJECT_IS_TOP_LEVEL})
option(COFFEE_BUILD_TESTS "Build Coffee tests" ${PROJECT_IS_TOP_LEVEL})
option(COFFEE_INSTALL "Generate Coffee install rules" ON)
option(COFFEE_WARNINGS_AS_ERRORS "Treat Coffee warnings as errors" OFF)
option(COFFEE_BUILD_VULKAN_MODULE "Build the optional CoffeeVulkan shared library" OFF)
option(COFFEE_ENABLE_STB_IMAGE "Compile stb_image into Coffee when its header is available" ON)
option(COFFEE_ENABLE_LTO "Enable release link-time optimization when supported" ON)
option(COFFEE_LEAN_SDL "Disable unused SDL subsystems in the embedded runtime" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Coffee's C++ ABI crosses the DLL boundary, so the engine, SDL, and clients
# must use the matching dynamic MSVC runtime.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

include(GNUInstallDirs)

# SDL is linked statically into Coffee.dll. An SDL3.dll import library cannot
# satisfy this because it contains no SDL implementation code.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL3/CMakeLists.txt")
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(SDL_DISABLE_INSTALL_DOCS ON CACHE BOOL "" FORCE)

    # Coffee currently owns windowing, input events, OpenGL, Vulkan surfaces,
    # clipboard, timing, and filesystem paths. Building unrelated SDL systems
    # into Coffee.dll only increases download size. Consumers that deliberately
    # use native SDL escape hatches can opt back into the complete SDL build.
    if(COFFEE_LEAN_SDL)
        foreach(subsystem IN ITEMS
                AUDIO CAMERA DIALOG GPU HAPTIC HIDAPI JOYSTICK POWER RENDER
                SENSOR TRAY NOTIFICATION PROCESS)
            set(SDL_${subsystem} OFF CACHE BOOL "" FORCE)
        endforeach()
        set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
    endif()

    add_subdirectory(deps/SDL3 EXCLUDE_FROM_ALL)
elseif(NOT TARGET SDL3::SDL3-static)
    find_package(SDL3 3.2 CONFIG QUIET COMPONENTS SDL3-static)
endif()

if(NOT TARGET SDL3::SDL3-static)
    message(FATAL_ERROR
        "Coffee requires the SDL3 static target so SDL can be embedded in Coffee.dll.\n"
        "Place the SDL3 source tree (including CMakeLists.txt) in deps/SDL3, or install an\n"
        "SDL3 package that provides SDL3::SDL3-static. The SDL3.lib shipped beside\n"
        "SDL3.dll is only an import library and cannot be embedded into Coffee.dll.")
endif()

set(COFFEE_GLAD_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/src/gl.c")
set(COFFEE_GLAD_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include/glad/gl.h")
if(NOT EXISTS "${COFFEE_GLAD_SOURCE}" OR NOT EXISTS "${COFFEE_GLAD_HEADER}")
    message(FATAL_ERROR
        "Coffee's bundled GLAD 2 loader is incomplete. Expected:\n"
        "  deps/glad/src/gl.c\n"
        "  deps/glad/include/glad/gl.h\n"
        "Re-extract the complete Coffee source archive before configuring.")
endif()

set(COFFEE_HAS_STB_IMAGE 0)
set(COFFEE_STB_INCLUDE "")
if(COFFEE_ENABLE_STB_IMAGE)
    find_path(COFFEE_STB_INCLUDE stb_image.h
        HINTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/stb"
        NO_DEFAULT_PATH)
    if(COFFEE_STB_INCLUDE)
        set(COFFEE_HAS_STB_IMAGE 1)
    else()
        message(STATUS "stb_image.h not found; Coffee will retain native BMP loading only.")
    endif()
endif()

set(COFFEE_HAS_VULKAN_MODULE 0)
if(COFFEE_BUILD_VULKAN_MODULE)
    find_package(Vulkan REQUIRED)
    set(COFFEE_HAS_VULKAN_MODULE 1)
endif()

# Keep the 0.3 capability macro as a source-compatibility alias.
set(COFFEE_HAS_VULKAN ${COFFEE_HAS_VULKAN_MODULE})
configure_file(cmake/config.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/coffee/config.h" @ONLY)

set(COFFEE_CORE_SOURCES
    src/application.cpp
    src/asset_locator.cpp
    src/buffer.cpp
    src/clock.cpp
    src/file.cpp
    src/framebuffer.cpp
    src/image.cpp
    src/input.cpp
    src/log.cpp
    src/mesh.cpp
    src/orthographic_camera.cpp
    src/perspective_camera.cpp
    src/physics_world_2d.cpp
    src/renderer.cpp
    src/shader.cpp
    src/shader_preprocessor.cpp
    src/sprite_batch.cpp
    src/texture.cpp
    src/vertex_array.cpp
    src/window.cpp
    ${COFFEE_GLAD_SOURCE})
if(COFFEE_HAS_STB_IMAGE)
    list(APPEND COFFEE_CORE_SOURCES src/stb_image_impl.cpp)
endif()

function(coffee_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /permissive- /Zc:__cplusplus /utf-8
            # The supported ABI requires a matching MSVC runtime/toolset.
            /wd4251 /wd4275)
        if(COFFEE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(COFFEE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(coffee_enable_lto target)
    if(NOT COFFEE_ENABLE_LTO)
        return()
    endif()
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT reason LANGUAGES C CXX)
    if(supported)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
    else()
        message(STATUS "LTO is unavailable for ${target}: ${reason}")
    endif()
endfunction()

# The default engine is one shared library and one compilation of its sources.
add_library(coffee SHARED ${COFFEE_CORE_SOURCES})
add_library(coffee::coffee ALIAS coffee)
add_library(coffee::shared ALIAS coffee) # 0.3 source compatibility

set_target_properties(coffee PROPERTIES
    OUTPUT_NAME Coffee
    EXPORT_NAME coffee
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES)

target_compile_features(coffee PUBLIC cxx_std_20)
target_compile_definitions(coffee PRIVATE COFFEE_BUILDING_CORE=1)
target_include_directories(coffee
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include"
        "${COFFEE_STB_INCLUDE}")
target_link_libraries(coffee PRIVATE SDL3::SDL3-static)
coffee_enable_warnings(coffee)
coffee_enable_lto(coffee)

# Vulkan remains an explicit companion DLL and never enlarges Coffee.dll.
if(COFFEE_BUILD_VULKAN_MODULE)
    add_library(coffee_vulkan SHARED src/vulkan_context.cpp)
    add_library(coffee::vulkan ALIAS coffee_vulkan)
    set_target_properties(coffee_vulkan PROPERTIES
        OUTPUT_NAME CoffeeVulkan
        EXPORT_NAME vulkan
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)
    target_compile_features(coffee_vulkan PUBLIC cxx_std_20)
    target_compile_definitions(coffee_vulkan PRIVATE COFFEE_BUILDING_VULKAN=1)
    target_include_directories(coffee_vulkan PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(coffee_vulkan PRIVATE coffee Vulkan::Vulkan)
    coffee_enable_warnings(coffee_vulkan)
    if(NOT MSVC)
        target_compile_options(coffee_vulkan PRIVATE -Wno-missing-field-initializers)
    endif()
    coffee_enable_lto(coffee_vulkan)
endif()

if(COFFEE_BUILD_EXAMPLES)
    foreach(example IN ITEMS application sandbox triangle physics_2d demo_3d)
        add_executable(coffee_${example} examples/${example}.cpp)
        target_link_libraries(coffee_${example} PRIVATE coffee::coffee)
    endforeach()
    if(COFFEE_BUILD_VULKAN_MODULE)
        add_executable(coffee_vulkan_clear examples/vulkan_clear.cpp)
        target_link_libraries(coffee_vulkan_clear PRIVATE coffee::coffee coffee::vulkan)
    endif()
endif()

if(COFFEE_BUILD_TESTS)
    include(CTest)
    foreach(test_name IN ITEMS core math physics shader_preprocessor)
        add_executable(coffee_${test_name}_tests tests/${test_name}_tests.cpp)
        target_link_libraries(coffee_${test_name}_tests PRIVATE coffee::coffee)
        add_test(NAME coffee_${test_name}_tests COMMAND coffee_${test_name}_tests)
    endforeach()
endif()

if(COFFEE_INSTALL)
    include(CMakePackageConfigHelpers)

    install(TARGETS coffee EXPORT CoffeeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT CoffeeTargets
        FILE CoffeeTargets.cmake
        NAMESPACE coffee::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Coffee")

    if(COFFEE_BUILD_VULKAN_MODULE)
        install(TARGETS coffee_vulkan EXPORT CoffeeVulkanTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        install(EXPORT CoffeeVulkanTargets
            FILE CoffeeVulkanTargets.cmake
            NAMESPACE coffee::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Coffee")
    endif()

    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/coffee/config.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/coffee")
    install(FILES README.md LICENSE.txt THIRD_PARTY_NOTICES.md
        DESTINATION ${CMAKE_INSTALL_DATADIR}/Coffee)
    install(DIRECTORY docs/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/Coffee/docs
        FILES_MATCHING PATTERN "*.md")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL3/LICENSE.txt")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL3/LICENSE.txt"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/Coffee/licenses"
            RENAME SDL3-LICENSE.txt)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/LICENSE")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/LICENSE"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/Coffee/licenses"
            RENAME GLAD-LICENSE.txt)
    endif()

    configure_package_config_file(
        cmake/CoffeeConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/CoffeeConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Coffee")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/CoffeeConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CoffeeConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/CoffeeConfigVersion.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CoffeeHelpers.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Coffee")
endif()

set(CPACK_PACKAGE_NAME "Coffee")
set(CPACK_PACKAGE_VENDOR "Coffee contributors")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "Coffee-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "ZIP")
include(CPack)
