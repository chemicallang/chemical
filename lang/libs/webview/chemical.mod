module webview

source "src"

import cstd
import std

// Local search path holding symlinks to the installed runtime libraries
// (e.g. libwebkit2gtk-4.1.so.0). Needed on systems where only the runtime
// .so.0 files are present without the -dev packages; a harmless no-op
// (unused -L) on systems with the dev packages installed.
link path "build/syslibs" if linux

link "glib-2.0" if linux
link "gobject-2.0" if linux
link "gtk-3" if linux
link "webkit2gtk-4.1" if linux
link "javascriptcoregtk-4.1" if linux
