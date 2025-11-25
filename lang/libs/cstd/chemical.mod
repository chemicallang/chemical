module cstd

source "src"
source "windows" if windows
source "posix" if !windows

link "libcmt" if windows && !tcc
link "legacy_stdio_definitions" if windows && !tcc
link "c" if linux