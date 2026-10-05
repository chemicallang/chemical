// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level symbol records (functions, globals, externals, intrinsics,
// types, aliases). Names are interned into a single byte pool.
// See mir-design.md §5 and mir-implementation-plan.md §2.5.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
#include <vector>

namespace mir {

enum class MIRSymbolKind : uint8_t {
    Function = 0,
    Global,
    External,
    Intrinsic,
    Type,
    Alias,
};

enum class MIRLinkage : uint8_t {
    Internal = 0,
    External,
    Weak,
    LinkOnce,
};

struct MIRSymbolRecord {
    MIRSymbolKind kind = MIRSymbolKind::Function;
    MIRLinkage linkage = MIRLinkage::Internal;
    TypeId type = MIR_INVALID_ID;
    uint32_t name_offset = 0;
    uint32_t name_length = 0;
    uint32_t mangled_offset = 0;
    uint32_t mangled_length = 0;
    uint32_t decl = MIR_INVALID_ID; // source declaration id
};

struct MIRSymbolTable {
    std::vector<MIRSymbolRecord> symbols;
    std::vector<char> names;

    const MIRSymbolRecord& get(SymbolId id) const { return symbols[id]; }
    uint32_t size() const { return static_cast<uint32_t>(symbols.size()); }

    SymbolId add(MIRSymbolRecord r,
                 const char* name, uint32_t name_len,
                 const char* mangled, uint32_t mangled_len) {
        r.name_offset = static_cast<uint32_t>(names.size());
        r.name_length = name_len;
        if (name_len) names.insert(names.end(), name, name + name_len);
        r.mangled_offset = static_cast<uint32_t>(names.size());
        r.mangled_length = mangled_len;
        if (mangled_len) names.insert(names.end(), mangled, mangled + mangled_len);
        symbols.push_back(r);
        return static_cast<SymbolId>(symbols.size() - 1);
    }

    const char* name_data(const MIRSymbolRecord& r) const {
        return names.data() + r.name_offset;
    }
    const char* mangled_data(const MIRSymbolRecord& r) const {
        return names.data() + r.mangled_offset;
    }
};

} // namespace mir
