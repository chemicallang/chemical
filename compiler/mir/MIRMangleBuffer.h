// Copyright (c) Chemical Language Foundation 2025.
//
// A per-thread mangling buffer. The legacy 2c visitor kept a single
// `BufferedWriter` alive for a whole translation; MIR constructs one (a
// `malloc(256)`) for every mangled name — per function, per global, per vtable
// slot. This hands out one reset buffer per thread instead.
//
// Mangling is a leaf operation: `NameMangler::mangle` writes into the buffer and
// never re-enters this helper, so a single buffer is safe. Callers copy the
// bytes out (into a `std::string` or another buffer) before the next use.

#pragma once

#include "std/chem_string_view.h"
#include "preprocess/2c/BufferedWriter.h"

namespace mir {

/** A reset, reused mangling buffer for the current thread. */
inline BufferedWriter& mangle_writer() {
    static thread_local BufferedWriter w(1024);
    w.setPositionUnsafely(0);
    return w;
}

} // namespace mir
