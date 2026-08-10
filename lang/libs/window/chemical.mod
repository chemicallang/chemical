module window

source "src"
source "win" if windows
source "posix" if !windows

import cstd
import std

// Windows: user32 = windowing; shell32 = drag&drop (DragQueryFileW); the
// default icon/UI bits come from user32 as well.
link "user32" if windows
link "shell32" if windows

// Linux: GTK3 + GDK
link ":libgtk-3.so.0" if linux
link ":libgdk-3.so.0" if linux
link ":libglib-2.0.so.0" if linux
link ":libgobject-2.0.so.0" if linux
