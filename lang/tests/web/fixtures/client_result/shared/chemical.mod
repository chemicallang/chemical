module shared

// Compiled by BOTH jobs, so it may not reach for a native-only symbol. The
// `std::Result` it returns is consumed differently by each side.
source "."

import std
