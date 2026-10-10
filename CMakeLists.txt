cmake_minimum_required(VERSION 3.16)
project(VoidProtocol VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(VP_HIDE_CONSOLE "Hide the console window on Windows (release builds)" OFF)

# ---------------------------------------------------------------------------
# raylib: сначала пробуем найти установленную (vcpkg / пакетный менеджер),
# иначе скачиваем и собираем версию 5.0 автоматически (нужны git и интернет).
# ---------------------------------------------------------------------------
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(VoidProtocol
    src/main.cpp
    src/Game.cpp
    src/World.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/Pickup.cpp
    src/Effects.cpp
    src/UI.cpp
)

target_include_directories(VoidProtocol PRIVATE src)
target_link_libraries(VoidProtocol PRIVATE raylib)

if(MSVC)
    target_compile_options(VoidProtocol PRIVATE /W4 /permissive- /utf-8)
    if(VP_HIDE_CONSOLE)
        set_target_properties(VoidProtocol PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    endif()
    # Рабочая папка для запуска из Visual Studio (рекорд сохраняется рядом с exe).
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VoidProtocol)
else()
    target_compile_options(VoidProtocol PRIVATE -Wall -Wextra)
    if(WIN32 AND VP_HIDE_CONSOLE)
        target_link_options(VoidProtocol PRIVATE -mwindows)
    endif()
endif()

if(APPLE)
    target_link_libraries(VoidProtocol PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()
