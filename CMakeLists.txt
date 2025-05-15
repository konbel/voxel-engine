cmake_minimum_required(VERSION 3.16)
project(voxel_engine)

set(CMAKE_CXX_STANDARD 20)

# OpenGL
find_package(OpenGL REQUIRED)

# SDL3
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(src/vendor/SDL3-3.2.14)

# GLEW
add_subdirectory(src/vendor/glew-2.1.0/build/cmake)

# ImGUI
add_subdirectory(src/vendor/imgui-1.91.9b)

add_executable(voxel_engine
    src/main.cpp

    src/vendor/stb-image/stb_image.cpp
)

target_include_directories(voxel_engine PRIVATE
    src/vendor/SDL3-3.2.14/include
    src/vendor/glm-1.0.1
    src/vendor/glm-1.0.1/gtc
    src/vendor/stb-image
    src/vendor/glew-2.1.0/include
    src/vendor/imgui-1.91.9b
)

target_link_libraries(voxel_engine PRIVATE
    OpenGL::GL
    SDL3::SDL3
    glew_s
    ImGui
)

add_custom_command(
    TARGET voxel_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/res
            ${CMAKE_CURRENT_BINARY_DIR}/res
)
