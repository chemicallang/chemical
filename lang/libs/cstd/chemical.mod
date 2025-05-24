module cstd

source "src"
source "windows" if windows
source "posix" if !windows