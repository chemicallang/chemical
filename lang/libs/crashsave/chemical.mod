module crashsave

source "win" if windows && !tcc
source "posix" if !windows && !tcc

// tcc setup
source "tcc/windows.ch" if tcc && windows
source "tcc/posix.ch" if tcc && !windows


import cstd
import std

link "dbghelp" if windows && !tcc
link "psapi" if windows && !tcc