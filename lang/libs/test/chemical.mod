module test

source "src"
source "win" if windows
source "posix" if posix

import cstd
import std
import test_env