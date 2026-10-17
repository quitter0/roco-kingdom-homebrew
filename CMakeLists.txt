cmake_minimum_required(VERSION 3.25)
project(roco-kingdom VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Keep the gnu17 dialect (C17 + GNU extensions) used by the project
set(CMAKE_C_EXTENSIONS ON)

add_subdirectory(vendor/raylib)

# Enable JPEG support in raylib (disabled by default, needed for GLB embedded textures)
target_compile_definitions(raylib PRIVATE SUPPORT_FILEFORMAT_JPG=1)

add_executable(roco-kingdom
    src/main.c
    src/player.c
    src/camera.c
    src/terrain.c
    src/pet.c
    src/battle.c
    src/npc.c
    src/inventory.c
    src/ui_menu.c
    src/audio.c
    src/save.c
    src/models_3d.c
    src/shop.c
    src/magic_spring.c
    resources/icon.rc
)

target_include_directories(roco-kingdom PRIVATE include vendor/raylib/src/external)
target_link_libraries(roco-kingdom raylib)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(roco-kingdom PRIVATE -Wall -Wextra)
endif()

file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})
