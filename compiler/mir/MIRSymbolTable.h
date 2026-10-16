// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level symbol records (functions, globals, externals, intrinsics,
// types, aliases). Names are interned into a single byte pool.
// See mir-design.md §5 and mir-implementation-plan.md §2.5.

#pragma once

#include "MIRTypes.h"
#include "MIRBytePool.h"

#include <cstdint>
#include <mutex>
#include <string>
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
    /** `names` is reference-stable on append (action E3, see MIRBytePool.h);
     * `symbols` stays a vector since only opaque ids escape it. */
    std::vector<MIRSymbolRecord> symbols;
    MIRBytePool names;

    const MIRSymbolRecord& get(SymbolId id) const { return symbols[id]; }
    uint32_t size() const { return static_cast<uint32_t>(symbols.size()); }

    /**
     * Intern a symbol record.
     *
     * Thread-safety (action E1): `add` is the only mutator and it is
     * mutex-guarded, so several lowering workers can intern concurrently. The
     * ids/records are *not* deduplicated — the backends match symbols by mangled
     * name, not by id (see `mangled_name`), so duplicate ids for one name are
     * already tolerated. Readers must consume a `get`/`mangled_name` result
     * without an intervening `add` on another thread reallocating the pools;
     * callers that lower and emit in the same thread (the planned worker model)
     * satisfy this.
     */
    SymbolId add(MIRSymbolRecord r,
                 const char* name, uint32_t name_len,
                 const char* mangled, uint32_t mangled_len) {
        std::lock_guard<std::mutex> lock(mtx_);
        r.name_offset = names.append(name, name_len);
        r.name_length = name_len;
        r.mangled_offset = names.append(mangled, mangled_len);
        r.mangled_length = mangled_len;
        symbols.push_back(r);
        return static_cast<SymbolId>(symbols.size() - 1);
    }

    /** Drop every record and name (used when a `MIRModule` is reused). */
    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        symbols.clear();
        names.clear();
    }

    const char* name_data(const MIRSymbolRecord& r) const {
        return names.data_at(r.name_offset);
    }
    const char* mangled_data(const MIRSymbolRecord& r) const {
        return names.data_at(r.mangled_offset);
    }

    /**
     * The symbol's linker name (its mangled spelling, falling back to the plain
     * name). LLVM globals are name-unique while MIR symbol ids are not — the
     * lowerer may intern the same function twice — so name-based lookups are
     * the ones that match what the linker sees.
     */
    std::string mangled_name(const MIRSymbolRecord& r) const {
        return std::string(r.mangled_length ? mangled_data(r) : name_data(r),
                           r.mangled_length ? r.mangled_length : r.name_length);
    }

private:
    /** Guards `add`/`clear` (action E1: shared interning under parallel lowering). */
    std::mutex mtx_;
};

} // namespace mir
