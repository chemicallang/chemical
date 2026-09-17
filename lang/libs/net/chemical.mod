module net

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import std
import core
import async

link "ws2_32" if windows
link "mswsock" if windows
