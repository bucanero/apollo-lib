cmake_minimum_required(VERSION 3.16)
project(apollo_patcher_gui C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Repo root (this file lives in <root>/gui).
set(APOLLO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(MBEDTLS_DIR "${APOLLO_ROOT}/mbedtls-2.16.12")

option(APOLLO_BIGENDIAN "Build the PS3 big-endian engine variant" OFF)

# ---------------------------------------------------------------------------
# libapollo engine (compiled from source/ so the GUI is self-contained).
# ---------------------------------------------------------------------------
set(ENGINE_SRCS
    ${APOLLO_ROOT}/source/crc_util.c
    ${APOLLO_ROOT}/source/list.c
    ${APOLLO_ROOT}/source/offzip.c
    ${APOLLO_ROOT}/source/packzip.c
    ${APOLLO_ROOT}/source/patches.c
    ${APOLLO_ROOT}/source/decrypt.c
    ${APOLLO_ROOT}/source/loader.c
    ${APOLLO_ROOT}/source/wildcard.c
    ${APOLLO_ROOT}/source/upy.c)

add_library(apollo_engine STATIC ${ENGINE_SRCS} src/apollo_ctrl.c)
target_include_directories(apollo_engine PUBLIC
    ${APOLLO_ROOT}/include
    ${APOLLO_ROOT}/tools          # provides dbglogger.h prototype
    ${MBEDTLS_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src)  # apollo_ctrl.h + app sources
target_compile_definitions(apollo_engine PRIVATE _GNU_SOURCE _USE_MBEDTLS)
if(APOLLO_BIGENDIAN)
    target_compile_definitions(apollo_engine PRIVATE __PS3_PC__)
endif()

# mbedcrypto: prefer the prebuilt static lib (built by CI), else expect a
# system/library target. Adjust to taste for your packaging.
find_library(MBEDCRYPTO_LIB
    NAMES mbedcrypto libmbedcrypto
    PATHS ${MBEDTLS_DIR}/build/library
    NO_DEFAULT_PATH)
if(NOT MBEDCRYPTO_LIB)
    find_library(MBEDCRYPTO_LIB NAMES mbedcrypto)
endif()
if(NOT MBEDCRYPTO_LIB)
    message(FATAL_ERROR "libmbedcrypto not found. Build mbedtls-2.16.12 first "
                        "(see CI: cd mbedtls-2.16.12/library && make static).")
endif()

# Link zlib directly (-lz) like the CLI Makefile does, rather than via
# find_package(ZLIB) — the latter fails under MSYS2/MinGW CI even though the
# compiler links -lz fine (same as the working CLI build). zlib.h is on the
# compiler's default include path everywhere the CLI builds.
target_link_libraries(apollo_engine PUBLIC ${MBEDCRYPTO_LIB} z m)

# ---------------------------------------------------------------------------
# Headless parity/CLI-equivalent test (no GUI deps) — always buildable.
# ---------------------------------------------------------------------------
add_executable(apollo_ctrl_test src/test_ctrl.c)
target_link_libraries(apollo_ctrl_test PRIVATE apollo_engine)

# ---------------------------------------------------------------------------
# Dear ImGui + GLFW GUI. Fetched at configure time (needs network once).
# ---------------------------------------------------------------------------
option(APOLLO_BUILD_GUI "Build the Dear ImGui GUI (needs OpenGL/GLFW)" ON)
if(APOLLO_BUILD_GUI)
    include(FetchContent)

    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.5)
    FetchContent_MakeAvailable(imgui)

    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    # X11 only: GLFW 3.4 builds the Wayland backend too by default, which needs
    # wayland-scanner + Wayland dev packages we don't install (and Wayland has no
    # window-icon protocol anyway). Keeps the Linux build's deps to xorg-dev.
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)

    # portable-file-dialogs: header-only native dialogs. On macOS/Linux it
    # shells out to osascript/zenity (a separate process), which sidesteps the
    # in-process NSOpenPanel breakage caused by GLFW's Cocoa init — so no custom
    # helper binary is needed. On Windows it uses the Win32 API directly.
    # File dialogs: portable-file-dialogs is vendored as a single header at
    # src/portable-file-dialogs.h (found via main.cpp's own directory).

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui PUBLIC glfw)

    find_package(OpenGL REQUIRED)

    add_executable(apollo_patcher_gui src/main.cpp)
    target_link_libraries(apollo_patcher_gui PRIVATE apollo_engine imgui OpenGL::GL)

    if(MINGW)
        # -static: statically link the MinGW runtime (libgcc / libstdc++ /
        #   winpthread) so the .exe runs on stock Windows without shipping DLLs.
        #   Windows system DLLs (opengl32, gdi32, comdlg32, …) are still linked
        #   via their import libs, which is correct.
        # -mwindows: build as a GUI-subsystem app so no console window opens
        #   behind the GUI. MinGW's WinMain shim still calls our main().
        target_link_options(apollo_patcher_gui PRIVATE -static -mwindows)
    endif()

    if(APPLE)
        # Bundle the .icns so Finder/Dock/⌘-Tab show the app icon.
        set(APP_ICNS ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.icns)
        target_sources(apollo_patcher_gui PRIVATE ${APP_ICNS})
        set_source_files_properties(${APP_ICNS} PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
        set_target_properties(apollo_patcher_gui PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "Apollo Patcher"
            MACOSX_BUNDLE_ICON_FILE icon.icns)
    endif()
endif()
