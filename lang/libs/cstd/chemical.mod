module cstd

source "src"
source "windows" if windows
source "posix" if !windows

link "libcmt" if windows
link "legacy_stdio_definitions" if windows