// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level vtable declaration records — the target-neutral description of an
// interface's vtable *type* (per interface) and its *definitions* (per
// interface+impl). Built by the front-end (`mir::build_module_vtables`,
// MIRVtable.h) so a backend can emit vtables without walking the AST: the C
// declaration pass and, importantly, the LLVM driver (`mir_emit_llvm_module`)
// both read these records.
//
// AST-free on purpose (ids + names only), so it lives in MIR core and is
// included by MIRModule.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mir {

/**
 * One vtable slot: the method's name, its self-erased function signature, and —
 * for a *definition* — the resolved callee's mangled name (empty means the slot
 * is a null function pointer, as a non-overridden primitive-impl method is).
 */
struct MIRVtableSlot {
    std::string name;
    TypeId type = MIR_INVALID_ID;
    std::string target;
};

/** A vtable type (kind 0) or definition (kind 1 container, 2 primitive). */
struct MIRVtableRecord {
    /** The vtable struct type name (types) or the global's mangled name (defs). */
    std::string name;
    /** The `__chx_<iface>_vt_t` struct type name (defs; equals `name` for types). */
    std::string type_name;
    uint8_t kind = 0; // 0 = type, 1 = container impl, 2 = primitive impl
    std::vector<MIRVtableSlot> slots;
};

/** The module's vtable declarations (see MIRVtable.h for the builder). */
struct MIRVtableTable {
    std::vector<MIRVtableRecord> types;
    std::vector<MIRVtableRecord> defs;
    void clear() {
        types.clear();
        defs.clear();
    }
};

} // namespace mir
