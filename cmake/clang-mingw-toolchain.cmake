# LLVM-MinGW toolchain for CMake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compilers
set(CMAKE_C_COMPILER "${TOOLCHAIN_ROOT}/bin/x86_64-w64-mingw32-clang.exe")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_ROOT}/bin/x86_64-w64-mingw32-clang++.exe")

# Resource compiler
set(CMAKE_RC_COMPILER "${TOOLCHAIN_ROOT}/bin/x86_64-w64-mingw32-windres.exe")

# Binutils
set(CMAKE_AR "${TOOLCHAIN_ROOT}/bin/x86_64-w64-mingw32-ar.exe")
set(CMAKE_RANLIB "${TOOLCHAIN_ROOT}/bin/x86_64-w64-mingw32-ranlib.exe")
set(CMAKE_STRIP "${TOOLCHAIN_ROOT}/bin/x86_64-w64-mingw32-strip.exe")

# Where to search for headers/libraries
set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_ROOT}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)