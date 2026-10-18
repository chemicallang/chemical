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

#include <cstdint>
#include <string>
#include <vector>

#include "std/chem_string_view.h"
#include "preprocess/2c/BufferedWriter.h"

namespace mir {

/** A reset, reused mangling buffer for the current thread. */
inline BufferedWriter& mangle_writer() {
    static thread_local BufferedWriter w(1024);
    w.setPositionUnsafely(0);
    return w;
}

/**
 * A reset, reused string for the current thread, one per nesting depth.
 *
 * Some spellings need a piece of text as one contiguous span *before* it can be
 * written to the output (a nested declarator is the main case: `(*name)(params)`
 * has to exist as a whole name before the return type can be spelled in front
 * of it). Lowering and emission must not allocate a `std::string` per such
 * name — a declaration path visits them per parameter, per member, per Alloca —
 * so the buffer is reset and reused instead.
 *
 * Depth-indexed because the user recurses: a nested declarator acquires its
 * buffer at `depth + 1` before it reads the one its caller is still holding.
 * The pool never grows, so a reference handed out stays valid, and depths past
 * the last slot share it (function-pointer nesting never goes that deep).
 */
inline std::string& decl_scratch(uint32_t depth) {
    static thread_local std::vector<std::string> pool(16);
    std::string& s = pool[depth < pool.size() ? depth : pool.size() - 1];
    s.clear();
    return s;
}

} // namespace mir
