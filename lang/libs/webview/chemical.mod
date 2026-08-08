module webview

source "src"

import cstd
import std

// link against the exact production runtime sonames (the versioned .so.0 files
// shipped by the runtime packages) — no -dev packages or copied libraries needed
link ":libglib-2.0.so.0" if linux
link ":libgobject-2.0.so.0" if linux
link ":libgtk-3.so.0" if linux
link ":libwebkit2gtk-4.1.so.0" if linux
link ":libjavascriptcoregtk-4.1.so.0" if linux
