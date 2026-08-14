module tls

source "src"
source "win" if windows

import cstd
import std
import net
import crypto
import encoding
import datetime

link "bcrypt" if windows

