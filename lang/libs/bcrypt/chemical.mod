module bcrypt

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import std

link "advapi32" if windows