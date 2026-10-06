include_guard(GLOBAL)

# Copy Coffee's runtime DLL beside an existing executable after each build.
function(coffee_copy_runtime target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "coffee_copy_runtime expected an existing target: ${target}")
    endif()
    if(WIN32)
        add_custom_command(TARGET "${target}" POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:coffee::coffee>"
                "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying Coffee runtime for ${target}")
    endif()
endfunction()

# Create a C++20 executable, link Coffee, and stage Coffee.dll on Windows.
# Usage: coffee_add_executable(my_game src/main.cpp src/game.cpp)
function(coffee_add_executable target)
    if(ARGC LESS 2)
        message(FATAL_ERROR "coffee_add_executable requires a target and at least one source file")
    endif()
    add_executable("${target}" ${ARGN})
    target_compile_features("${target}" PRIVATE cxx_std_20)
    target_link_libraries("${target}" PRIVATE coffee::coffee)
    coffee_copy_runtime("${target}")
endfunction()
