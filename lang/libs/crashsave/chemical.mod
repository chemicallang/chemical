module crashsave

source "win" if windows && !tcc
source "tcc" if tcc

import cstd
import std