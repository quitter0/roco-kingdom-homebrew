cmake_minimum_required(VERSION 3.25)
project(roco-kingdom VERSION 1.0.1 LANGUAGES C)

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

# Portable release ZIP: cmake --build <build> --target package_portable
# CPack is not used because raylib unconditionally registers install() rules
# for its own headers/libraries, which would leak into the archive.
set(PACKAGE_NAME "roco-kingdom-homebrew-${PROJECT_VERSION}-windows-x64")
set(PACKAGE_ROOT "${CMAKE_BINARY_DIR}/package")
set(PACKAGE_DIR "${PACKAGE_ROOT}/${PACKAGE_NAME}")

add_custom_target(package_portable
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${PACKAGE_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}/resources"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:roco-kingdom>" "${PACKAGE_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources" "${PACKAGE_DIR}/resources"
    COMMAND ${CMAKE_COMMAND} -E rm -f
        "${PACKAGE_DIR}/resources/save.dat"
        "${PACKAGE_DIR}/resources/icon.rc"
        "${PACKAGE_DIR}/resources/logo.ico"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/packaging/README.txt"
        "${CMAKE_SOURCE_DIR}/LICENSE"
        "${CMAKE_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
        "${PACKAGE_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E rm -f
        "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.zip"
    COMMAND ${CMAKE_COMMAND} -E chdir "${PACKAGE_ROOT}"
        ${CMAKE_COMMAND} -E tar cf
        "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.zip" --format=zip -- "${PACKAGE_NAME}"
    DEPENDS roco-kingdom
    COMMENT "Packaging ${PACKAGE_NAME}.zip"
    VERBATIM
)
