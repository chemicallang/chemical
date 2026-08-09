module process

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import std
