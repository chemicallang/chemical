module async

source "src"
source "posix" if !windows
source "win" if windows

import cstd
import core
import std
