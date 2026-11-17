module shared

// Models and pure logic: compiled by BOTH jobs, so it may not reach for
// `page`, `dom`, the HTML/CSS/JS macros or any native-only symbol. The JS
// backend reports a native symbol here with its source location.
source "."

import std
