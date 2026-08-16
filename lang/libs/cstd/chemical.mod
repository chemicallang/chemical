module cstd

source "src"
source "windows" if windows
source "posix" if !windows
source "posix_linux" if linux
source "posix_macos" if macos

link "libcmt" if windows && !tcc
link "legacy_stdio_definitions" if windows && !tcc
link "c" if linux