# VOID PROTOCOL

[🇷🇺 Русский](README.md) | [🇬🇧 English](README_en.md)

A 2D sci-fi top-down shooter roguelike built with **C++17 + raylib 5.0**.

Clear the station of hostile droids, collect essence, survive waves of enemies, and destroy the **Core** in the final room.

The game uses no external assets: all graphics are rendered using raylib primitives, so a single executable is all you need to run the game after building it.

## Controls

| Action | Keys |
|---|---|
| Movement | `W A S D` / Arrow keys |
| Aim | Mouse |
| Fire (hold) | Left mouse button |
| Reload | `R` |
| Dash (grants invulnerability frames) | `Space` / `Left Shift` |
| Pause | `Esc` / `P` |
| Menu navigation | Mouse, or `W/S` + `Enter` |
| Upgrade selection | `1` `2` `3`, mouse, or Arrow keys + `Enter` |

## How to Play

* Entering a new room locks its doors. Survive 1–3 waves of enemies to clear it.
* Enemies drop **essence** (experience), and sometimes health kits and ammunition. Level up to choose 1 of 3 upgrades.
* After clearing a room, a health kit, ammunition, and essence are left on the floor.
* The room farthest from the starting point is the boss lair (shown in pink on the minimap). The boss has 3 phases.
* If you run completely out of ammunition, an emergency condenser slowly replenishes your reserve ammo, preventing softlocks.
* Your high score is saved in `void_protocol.sav` next to the executable (in the current working directory).

## Project Structure

```text
VoidProtocol/
  CMakeLists.txt
  src/
    main.cpp      Entry point
    Common.h      Constants, vector math, RNG, color palette
    Game.h/.cpp   Game loop, state machine (MENU/PLAYING/PAUSED/UPGRADE/GAMEOVER/VICTORY),
                  room and wave logic, combat, camera, screen rendering
    World.h/.cpp  Tile map, procedural room and corridor generation on a grid,
                  circular wall collisions, visibility, distance field (BFS) for pathfinding
    Player.h/.cpp Player: movement, dash, weapons, reloading, leveling and upgrades
    Enemy.h/.cpp  Chaser, Shooter, Tank, Boss (the Core), and their AI
    Pickup.h/.cpp Health kits, ammunition, essence
    Projectile.h  Projectile
    Effects.h/.cpp Particles, screen shake, shockwaves
    UI.h/.cpp     HUD, minimap, menu buttons, upgrade cards, crosshair
```

## Building

Requirements: **CMake 3.16+**, a C++17 compiler (MSVC 2019+, MinGW-w64 GCC 9+, GCC 9+, or Clang 10+), and **Git + an internet connection for the initial configuration**.

CMake first looks for an installed version of raylib using `find_package(raylib 5.0)`. If none is found, it downloads and builds raylib 5.0 using `FetchContent`.

### Windows + Visual Studio (MSVC)

Open the **Developer Command Prompt for Visual Studio** or a regular PowerShell terminal:

```powershell
cd VoidProtocol
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\VoidProtocol.exe
```

(For Visual Studio 2019, use `-G "Visual Studio 16 2019"` instead.) You can also open the project folder directly in Visual Studio using **Open a local folder** — CMake integration will handle the configuration automatically.

### Windows + MinGW-w64

MinGW-w64 must be available in your `PATH`. For example, install it through MSYS2:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

Then build the game:

```bash
cd VoidProtocol
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/VoidProtocol.exe
```

With Ninja, use `-G Ninja` instead of `-G "MinGW Makefiles"`.

To hide the console window, add `-DVP_HIDE_CONSOLE=ON` during configuration.

### Linux (GCC / Clang)

The following system libraries are required to build raylib:

```bash
# Debian / Ubuntu
sudo apt install build-essential git cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev \
     libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev

# Fedora
sudo dnf install gcc-c++ git cmake alsa-lib-devel mesa-libGL-devel libX11-devel libXrandr-devel \
     libXi-devel libXcursor-devel libXinerama-devel

# Arch Linux
sudo pacman -S base-devel git cmake alsa-lib mesa libx11 libxrandr libxi libxcursor libxinerama
```

Build and run:

```bash
cd VoidProtocol
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/VoidProtocol
```

### macOS

```bash
xcode-select --install
brew install cmake
cd VoidProtocol
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/VoidProtocol
```

### If raylib Is Already Installed

* **vcpkg:** Run `vcpkg install raylib`, then configure the project with `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake`.
* **Linux/macOS:** Install the `raylib` package (or run `brew install raylib` on macOS). CMake will find it automatically.
* The version must be **5.0 or newer**. If you have raylib 4.x, upgrade it: some functions were renamed in version 5.0 (for example, `MOUSE_BUTTON_LEFT`).

### Troubleshooting

* **`Could not resolve host: github.com`** — An internet connection is required during the initial configuration to download raylib. Alternatively, install raylib beforehand (see above).
* **The window opens and immediately closes** — Run the game from a terminal and check the raylib output. The most common cause is an outdated graphics driver that does not support OpenGL 3.3.
* **The high score is not saved** — `void_protocol.sav` is written to the *current working directory*. Run the game from the directory containing the executable, or make sure you have write permissions there.
* **The UI text is in English** — raylib's default font does not support Cyrillic characters, and external fonts were intentionally omitted from the project.
