cmake_minimum_required(VERSION 3.16)
project(ImGui)

set(CMAKE_CXX_STANDARD 20)

add_library(ImGui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_sdl3.cpp
    backends/imgui_impl_opengl3.cpp
)

target_include_directories(ImGui PRIVATE
    .
    ../SDL3-3.2.14/include
)