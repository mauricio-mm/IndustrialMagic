cmake_minimum_required(VERSION 3.20)

project(IndustrialMagic VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(raylib STATIC IMPORTED)
set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylib.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_executable(IndustrialMagic
    src/main.cpp
    src/environment3d.cpp
    src/pixel_art_shader.cpp
    src/terrain/terrain_draw.cpp
    src/terrain/terrain_geometry.cpp
    src/terrain/terrain_noise.cpp
    src/terrain/terrain_plane.cpp
    src/terrain/terrain_render.cpp
    src/terrain/terrain_panel.cpp
)

target_include_directories(IndustrialMagic PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_options(IndustrialMagic PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wformat=2
    -Wcast-align
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-O0>
)

target_link_libraries(IndustrialMagic PRIVATE
    raylib
    opengl32
    gdi32
    winmm
)
