library ui_native

source "src"

import cstd
import std
import fs
import path
import environment
import ui
import window

link ":libcairo.so.2" if linux
