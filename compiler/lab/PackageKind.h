// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <cstdint>

enum class PackageKind : uint8_t {
    Library,      // main function is mangled (default)
    Application,   // main function is no_mangle (entry point)
    Last = Application
};