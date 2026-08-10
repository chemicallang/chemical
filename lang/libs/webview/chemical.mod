module webview

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import std
import window

// Win32: pure-C WebView2 backend (lang/libs/webview/win/win.ch).
// Reference build of the ported C:
//   tcc wvwin.c -o file.exe -lshlwapi -lole32
// tcc.exe auto-links user32/shell32, but libtcc (used by TCCCompiler) does not
// include them in its defaults, so they must be listed explicitly.
link "shlwapi" if windows
link "ole32" if windows
link "user32" if windows
link "shell32" if windows

// link against the exact production runtime sonames (the versioned .so.0 files
// shipped by the runtime packages) — no -dev packages or copied libraries needed
link ":libglib-2.0.so.0" if linux
link ":libgobject-2.0.so.0" if linux
link ":libgtk-3.so.0" if linux
link ":libwebkit2gtk-4.1.so.0" if linux
link ":libjavascriptcoregtk-4.1.so.0" if linux
