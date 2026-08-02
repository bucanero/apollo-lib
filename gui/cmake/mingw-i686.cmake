# CMake toolchain: cross-compile for 32-bit Windows (x86) with mingw-w64 on
# Linux. MSYS2 dropped its 32-bit toolchain, so a genuine i686 Windows build is
# produced this way instead. Used by the "Windows x86 (Linux cross)" CI job.
#   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=.../gui/cmake/mingw-i686.cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(TOOLCHAIN i686-w64-mingw32)
set(CMAKE_C_COMPILER   ${TOOLCHAIN}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN}-g++)
set(CMAKE_RC_COMPILER  ${TOOLCHAIN}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN})
# Host tools (git for FetchContent, etc.) come from the host PATH. Libraries and
# headers may come from either the cross sysroot (zlib, opengl32, …) or explicit
# project paths (the repo-built libmbedcrypto.a), so search BOTH.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
