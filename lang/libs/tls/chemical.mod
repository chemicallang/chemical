module tls

source "src"
source "win" if windows

import cstd
import std
import net
import crypto
import encoding
import datetime
import osrand

link "bcrypt" if windows

