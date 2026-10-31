# Ona Launcher

Ona Launcher is a small PlayStation Vita homebrew launcher prototype. It is
inspired by the organization of iiSU/ZNeko, but the implementation and assets
in this repository are independent and contain no copied ZNeko code or assets.

The first prototype provides:

- A 960x544 Vita2D interface with Games, Homebrew Apps, Favorites, and
  Settings sections.
- D-pad, face-button, Start, and basic front-touch navigation.
- A small configuration file at `ux0:data/ona-launcher/config.ini`.
- A clean exit path through Start or the Vita home/quit event.

## Prerequisites

Building and installing require:

- [VitaSDK](https://vitasdk.org/) with `arm-vita-eabi-gcc`, Vita2D, and the
  CMake helpers available.
- [Vita3K](https://vita3k.org/) for desktop testing, or a PlayStation Vita
  with a homebrew environment for device testing.

This repository does not vendor VitaSDK or Vita3K. A local build cannot be
performed unless the `VITASDK` environment variable and its toolchain are
installed.

## Build

From the repository root:

```sh
export VITASDK=/path/to/vitasdk
make
```

The resulting `build/ona_launcher.vpk` can be installed with VitaShell by
copying it to the Vita and selecting it, or imported into Vita3K through its
install-package workflow.

The equivalent direct CMake commands are:

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake"
cmake --build build
```

For a clean rebuild:

```sh
rm -rf build
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake"
cmake --build build --parallel
```

## Controls

Use Up/Down or front-touch rows to select a section. Cross opens the selected
placeholder section, Circle returns to the main menu, and Start exits the
application cleanly.

The menu is intentionally a scaffold: game and homebrew discovery will be
implemented in a later iteration.

## Project layout

```text
src/main.c       Vita entry point, rendering, input, and config persistence
CMakeLists.txt   VitaSDK build and VPK packaging configuration
assets/          Independent placeholder asset notes
```
