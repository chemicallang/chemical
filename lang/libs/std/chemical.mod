module std

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import core