module test_libs_runner

source "main.ch"

import test
// why are these required here, so
// the test_runner function is comptime, it uses std::span, but since its comptime, we don't check it YET
// so std::span must be available at runtime, therefore a dependency on std
// similarly the test functions are located in these submodules, to access those, we need a direct dependency
// therefore we depend upon html and css for the declarations
import std
import "../html"
import "../css"