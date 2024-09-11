cmake_minimum_required(VERSION 3.0...3.25)

project(
    UI
    VERSION 0.1.0
    LANGUAGES CXX C
)

# Add GLFW
if (NOT EMSCRIPTEN)
    add_subdirectory(glfw)
else()
    # Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

# Add WebGPU and GLFW extension
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

# Set directories for source and include files
set(SRC_DIR src)
set(INCLUDE_DIR include)

# Include directories for headers
include_directories(${INCLUDE_DIR})

# Collect all source files in the src folder
file(GLOB_RECURSE SOURCE_FILES ${SRC_DIR}/*.cpp ${SRC_DIR}/*.c)

# Add executable using the source files
add_executable(App
    ${SOURCE_FILES}
)

# Add glfw and glfw3webgpu as dependencies of our App
target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)

# Copy WebGPU binaries if the function is defined
target_copy_webgpu_binaries(App)

# Set target properties
set_target_properties(App PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)

# Set compiler warnings based on the compiler
if (MSVC)
    target_compile_options(App PRIVATE /W4)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

# Xcode-specific properties
if (XCODE)
    set_target_properties(App PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()

# Emscripten-specific properties
if (EMSCRIPTEN)
    set_target_properties(App PROPERTIES SUFFIX ".html")
    target_link_options(App PRIVATE -sASYNCIFY)
endif()
