# VOID PROTOCOL

2D sci-fi top-down shooter roguelike на **C++17 + raylib 5.0**.
Очисти станцию от враждебных дроидов, собирай эссенцию, переживай волны и уничтожь **Главное Ядро** в последней комнате.

Игра не использует внешних ассетов: вся графика рисуется примитивами raylib, поэтому после сборки достаточно одного exe-файла.

## Управление

| Действие | Клавиши |
|---|---|
| Движение | `W A S D` / стрелки |
| Прицел | мышь |
| Огонь (удержание) | левая кнопка мыши |
| Перезарядка | `R` |
| Рывок (кадры неуязвимости) | `Space` / `Left Shift` |
| Пауза | `Esc` / `P` |
| Меню | мышь, либо `W/S` + `Enter` |
| Выбор улучшения | `1` `2` `3`, мышь, либо стрелки + `Enter` |

## Как играть

* Заходя в новую комнату, ты запираешь двери: нужно пережить 1-3 волны врагов.
* С врагов падает **эссенция** (опыт), иногда аптечки и патроны. Набрал уровень - выбери 1 из 3 улучшений.
* После зачистки комнаты на полу остаются аптечка, патроны и эссенция.
* Самая дальняя комната от старта - логово босса (на миникарте розовая). У босса 3 фазы.
* Если патроны полностью кончились, аварийный конденсатор медленно подзаряжает резерв, поэтому в тупик игра не заходит.
* Лучший результат хранится в файле `void_protocol.sav` рядом с exe (рабочая папка запуска).

## Структура проекта

```
VoidProtocol/
  CMakeLists.txt
  src/
    main.cpp      точка входа
    Common.h      константы, векторная математика, ГСЧ, палитра
    Game.h/.cpp   игровой цикл, стейт-машина (MENU/PLAYING/PAUSED/UPGRADE/GAMEOVER/VICTORY),
                  логика комнат и волн, бой, камера, отрисовка экранов
    World.h/.cpp  тайловая карта, процедурная генерация комнат на сетке и коридоров,
                  круговые коллизии со стенами, видимость, поле расстояний (BFS) для поиска пути
    Player.h/.cpp игрок: движение, рывок, оружие, перезарядка, прокачка
    Enemy.h/.cpp  Chaser (таран), Shooter (стрелок), Tank (тяжёлый), Boss (Главное Ядро) и их ИИ
    Pickup.h/.cpp аптечки, патроны, эссенция
    Projectile.h  снаряд
    Effects.h/.cpp частицы, тряска экрана (screen shake), ударные волны
    UI.h/.cpp     HUD, миникарта, кнопки меню, карточки улучшений, прицел
```

## Сборка

Нужны: **CMake 3.16+**, компилятор C++17 (MSVC 2019+, MinGW-w64 GCC 9+, GCC 9+, Clang 10+) и **git + интернет при первой конфигурации**.
CMake сам ищет установленный raylib (`find_package(raylib 5.0)`), а если не находит - скачивает и собирает raylib 5.0 через `FetchContent`.

### Windows + Visual Studio (MSVC)

Откройте "Developer Command Prompt for VS" или обычный PowerShell:

```powershell
cd VoidProtocol
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\VoidProtocol.exe
```

(Для VS 2019 используйте `-G "Visual Studio 16 2019"`.) Можно просто открыть папку проекта в Visual Studio ("Open a local folder") - CMake подхватится сам.

### Windows + MinGW-w64

Нужен MinGW-w64 в `PATH` (например, из MSYS2: `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja`).

```bash
cd VoidProtocol
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/VoidProtocol.exe
```

С Ninja: `-G Ninja` вместо `-G "MinGW Makefiles"`.
Чтобы скрыть консольное окно: добавьте `-DVP_HIDE_CONSOLE=ON`.

### Linux (GCC / Clang)

Системные библиотеки, нужные raylib для сборки:

```bash
# Debian / Ubuntu
sudo apt install build-essential git cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev \
     libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
# Fedora
sudo dnf install gcc-c++ git cmake alsa-lib-devel mesa-libGL-devel libX11-devel libXrandr-devel \
     libXi-devel libXcursor-devel libXinerama-devel
# Arch
sudo pacman -S base-devel git cmake alsa-lib mesa libx11 libxrandr libxi libxcursor libxinerama
```

Сборка:

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

### Если raylib уже установлен

* vcpkg: `vcpkg install raylib`, затем `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake`
* Linux/macOS: пакет `raylib` (или `brew install raylib`) - CMake найдёт его автоматически.
* Версия должна быть **5.0 или новее**. Если используется 4.x, обновите raylib: часть функций изменила имена в 5.0 (например, `MOUSE_BUTTON_LEFT`).

### Частые проблемы

* **`Could not resolve host: github.com`** - при первой конфигурации нужен интернет (скачивание raylib). Либо поставьте raylib заранее (см. выше).
* **Окно открывается и сразу закрывается** - запустите из консоли и посмотрите вывод raylib; чаще всего это старый видеодрайвер без OpenGL 3.3.
* **Рекорд не сохраняется** - файл `void_protocol.sav` пишется в *текущую рабочую папку*; запускайте игру из папки с exe, либо с правами записи туда.
* Текст интерфейса английский: встроенный шрифт raylib не содержит кириллицу, а внешние шрифты в проект намеренно не добавлялись.
