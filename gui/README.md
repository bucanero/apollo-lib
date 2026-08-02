# Apollo Patcher GUI

A cross-platform (Windows / macOS / Linux) graphical front-end for the Apollo
`patcher` CLI, built on **Dear ImGui + GLFW/OpenGL3**.

It does not reimplement any patch logic: it wraps the existing `libapollo`
engine and drives the same functions the CLI does
(`load_patch_code_list`, `apply_cheat_patch_code`).

## Architecture

```
  main.cpp            Dear ImGui UI (file pickers, code list, option combos, log)
  apollo_ctrl.[ch]    stdio-free facade over libapollo — shared by GUI and CLI
  ../source/*.c       existing libapollo engine (unchanged)
  ../mbedtls-2.16.12  crypto backend (libmbedcrypto), same as the CLI
```

`apollo_ctrl` replaces the CLI's three UI-bound pieces with callbacks/data:

| CLI (patcher.c)              | GUI equivalent                                  |
|------------------------------|-------------------------------------------------|
| `printf` / `dbglogger_log`   | `apollo_set_log_sink()` → log panel             |
| `scanf` in `get_user_options`| `apollo_opt_set_selected()` ← combo boxes       |
| `is_active_code` arg parsing | per-row checkboxes                              |

MicroPython `print()` output is also routed to the log panel (the engine is
built **without** `-DAPOLLO_CLI`, so `dbglogger_printf` goes to the sink).

## Build

Prerequisites: CMake ≥ 3.16, a C/C++ toolchain, zlib, OpenGL, and a built
`libmbedcrypto` (the CI already builds it):

```bash
cd mbedtls-2.16.12/library && make static && mkdir -p ../build/library && \
  cp libmbedcrypto.a ../build/library && cd ../..
```

Then:

```bash
cd gui
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # fetches Dear ImGui + GLFW
cmake --build build -j
```

Targets:
- `apollo_patcher_gui` — the GUI app (macOS: `build/apollo_patcher_gui.app`)
- `apollo_ctrl_test`   — headless lister, proves parity with `patcher <file>`
- `-DAPOLLO_BUILD_GUI=OFF` builds only the engine + headless test (no GL needed)

## Packaging (per OS)

| OS      | Tooling                                             |
|---------|-----------------------------------------------------|
| macOS   | `MACOSX_BUNDLE` → `.app`, `cpack -G DragNDrop` (dmg) |
| Windows | MSYS2/MinGW (same as `build-win.yml`), `cpack -G NSIS` |
| Linux   | `cpack -G AppImage` / `.deb`                         |

CI is already wired: `.github/workflows/build.yml` (macOS + Linux) and
`build-win.yml` build the GUI after the CLI and upload it as an
`apollo-gui-<sha>-<os>` artifact (`.app` on macOS, `apollo_patcher_gui.exe`
elsewhere). Turn those artifacts into installers with CPack when you're ready.

**Windows architectures.** The `msys` job (MSYS2 MINGW64) produces the **x64**
build. The **x86 (32-bit)** build is cross-compiled on Linux with mingw-w64
(`win32-cross` job) using the toolchain file `gui/cmake/mingw-i686.cmake` —
MSYS2 dropped its 32-bit toolchain, so its MINGW32 target silently emitted x64
binaries. To reproduce the 32-bit build locally on Linux:

```bash
sudo apt-get install -y gcc-mingw-w64-i686 g++-mingw-w64-i686 libz-mingw-w64-dev ninja-build
# build mbedcrypto into mbedtls-2.16.12/build/library with the same toolchain file, then:
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/cmake/mingw-i686.cmake
cmake --build build
```

## Features

- Native file pickers via header-only
  [portable-file-dialogs](https://github.com/samhocevar/portable-file-dialogs),
  vendored as a single header at [src/portable-file-dialogs.h](src/portable-file-dialogs.h).
  On macOS/Linux it shells out to `osascript`/`zenity` (a separate process),
  which sidesteps the in-process `NSOpenPanel` breakage caused by GLFW's Cocoa
  init — so no custom helper binary or bundling is needed. Windows uses the
  Win32 dialog API directly. Patch files are filtered to `*.savepatch`.
- **Apply is blocked** while any checked code has an unfilled required option:
  the offending combo boxes and an "(required)" tag turn red, and the button is
  disabled until every selection is made.

## Known caveats / TODO

- **Big-endian (PS3):** endianness is a *compile-time* switch in the engine
  (`__PS3_PC__`). Build a second variant with `-DAPOLLO_BIGENDIAN=ON` (mirrors
  the CLI's `patcher-bigendian`) rather than toggling at runtime.
- **Linux dialogs:** portable-file-dialogs needs a dialog helper present at
  runtime (`zenity`, `kdialog`, `matedialog`, or `qarma`).
- **OpenGL / GPU-less & RDP hosts:** the app uses Dear ImGui's fixed-function
  `imgui_impl_opengl2` backend with a legacy context **on all platforms** (needs
  only OpenGL 1.1 — one code path everywhere; this 2D tool has no use for modern
  GL). On **Windows** it uses the **system OpenGL driver** by default (hardware
  when present). Some hosts have no usable OpenGL — **Remote Desktop exposes no
  WGL OpenGL at all**, and so do some VMs / GPU-less machines — and there the
  window can't be created (you'll get a message box saying so).

  **Software-OpenGL fallback (drop-in).** The Windows build ships a self-contained
  Mesa software renderer at **`softgl\opengl32.dll`** next to the exe. It is *not*
  used by default (it's in a subfolder), so the app uses the system GPU driver. If
  you hit the OpenGL error, **copy `softgl\opengl32.dll` up into the same folder
  as `apollo_patcher_gui.exe`** and relaunch — GLFW loads `opengl32.dll` from the
  exe's own directory first, so it then renders in software (presented via GDI,
  which works over RDP); `GALLIUM_DRIVER=llvmpipe` is set in `main` to force it.
  The bundled DLL is Mesa **17.2.6** — old enough to be a single self-contained
  file, and verified working on Windows 7, both x86 and x64. macOS/Linux never
  need this.

## Credits / third-party

- [portable-file-dialogs](https://github.com/samhocevar/portable-file-dialogs)
  by Sam Hocevar — native file dialogs (WTFPL). Vendored at
  `src/portable-file-dialogs.h`; update by replacing that file from upstream.
  Carries one **LOCAL PATCH** (search that string in the file): a 32-bit-only
  fix in `pfd::notify` where a capture-less lambda wouldn't bind to the
  `__stdcall ENUMRESNAMEPROC` on x86 — re-apply if you refresh the header.
- [Dear ImGui](https://github.com/ocornut/imgui) and
  [GLFW](https://github.com/glfw/glfw) — fetched at configure time via CMake.

## App icon

Source art: `assets/icon.png`. Derived files (checked in; the app needs no
image decoder at runtime):
- `assets/icon.icns` — macOS bundle icon (Dock/Finder), wired via CMake
  `MACOSX_BUNDLE_ICON_FILE`. Regenerate from the PNG with `sips`/`iconutil`.
- `src/icon_rgba_z.h` — the icon **pre-decoded to 256×256 RGBA and zlib-deflated**
  (262 KB → ~51 KB). Inflated at startup with zlib (already linked) and passed to
  `glfwSetWindowIcon()` for the Windows/Linux title-bar & taskbar (no-op on
  macOS — the whole path is behind `#ifndef __APPLE__`). Regenerate by resizing
  `assets/icon.png` to 256×256, decoding to raw RGBA, `compress2()`-ing at
  `Z_BEST_COMPRESSION`, and `xxd -i`-ing the deflated bytes into this header.
