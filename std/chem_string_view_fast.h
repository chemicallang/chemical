// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "chem_string_view.h"

struct FastHashInternedView {
    std::size_t operator()(chem::string_view const& k) const noexcept {
        // cast pointer to integer of pointer width
        auto p = reinterpret_cast<std::uintptr_t>(k.data());
        // turn into size_t (same width as uintptr_t on typical platforms)
        auto h = static_cast<std::size_t>(p);
        // mix in the other integer (k.tag) using boost::hash_combine style
        // this is cheap: one add, two shifts, one xor
        h ^= k.size() + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        // do a final xor-fold for 32-bit targets (keeps result well distributed)
        if constexpr (sizeof(std::size_t) == 4) {
            h ^= h >> 16;
        } else {
            h ^= h >> 32;
        }
        return h;
    }
};
struct FastEqInternedView {
    bool operator()(chem::string_view const& a, chem::string_view const& b) const noexcept {
        return a.data() == b.data() && a.size() == b.size();
    }
};