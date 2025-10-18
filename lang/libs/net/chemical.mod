module net

source "src"

import cstd
import std

link "ws2_32" if windows
link "mswsock" if windows