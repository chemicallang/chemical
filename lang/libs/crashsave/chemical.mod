module crashsave

source "win" if windows && !tcc
source "tcc" if tcc

import cstd
import std

link "dbghelp" if windows && !tcc
link "psapi" if windows && !tcc