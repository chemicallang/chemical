module uuid

source "src"

import cstd
import std
import atomic

link "advapi32" if windows
