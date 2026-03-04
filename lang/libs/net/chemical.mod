module net

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import std

link "ws2_32" if windows
link "mswsock" if windows