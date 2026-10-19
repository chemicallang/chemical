// Copyright (c) Chemical Language Foundation 2025.
//
// The declaration list (action A3, the declaration-path endgame). The C
// declaration pass used to walk the AST *and* print C in the same methods; it
// now records an ordered list of these items, and an AST-free printer
// (`MIRDeclPrinter::decl_list_c`) turns them into C. The builder stays the
// declaration-collection pass (allowed to see the AST); the printing does not.
//
// Only declarations live here — function *bodies*, global *definitions* and
// vtable *definitions* are still emitted inline by the implement pass (they are
// interleaved with lowered bodies), not through this list. Those definitions
// read MIR records too, though: a global's definition comes from the
// `MIRGlobal::decl` record the lowerer filled (`MIRGlobalDecl`, MIRGlobalDecl.h)
// and a vtable's from the shared `mir::build_vtable_def_record`
// (`MIRVtableRecord`, MIRVtableTable.h), neither from an AST re-read.
//
// AST-free (ids + names only), so it can be printed by the MIRTests-covered
// `MIRDeclPrinter`.

#pragma once

#include "MIRTypes.h"
#include "MIRPrototype.h"
#include "MIRGlobalDecl.h"
#include "MIRVtableTable.h"

#include <string>
#include <vector>

namespace mir {

enum class MIRDeclKind : uint8_t {
    ForwardStruct = 0, // `\nstruct <name>;`
    ForwardUnion,      // `\nunion <name>;`
    AggregateBody,     // a struct/union/variant body (from `aggregate_body_c`)
    Typedef,           // `\ntypedef <declarator>;`
    Prototype,         // `\n[extern ][static ][__chem_dllimport ]<proto>;`
    Global,            // `\n[__chx_thread_local ][static ][volatile ]<declarator>;`
    VtableType,        // `\ntypedef struct <name> { ... } <name>;`
    VtableExtern,      // `\nextern const <type_name> <name>;`
};

struct MIRDeclItem {
    MIRDeclKind kind = MIRDeclKind::ForwardStruct;

    /** Forward struct/union tag, typedef name, or vtable global name. */
    std::string name;
    /** The `__chx_<iface>_vt_t` type name (vtable items). */
    std::string type_name;
    /** Names the construct in the (unreachable) aggregate diagnostic comment. */
    std::string what;

    TypeId type = MIR_INVALID_ID;
    TypeId elem = MIR_INVALID_ID;
    bool unsized_array = false;

    MIRPrototype proto;   // Prototype
    MIRGlobalDecl global; // Global
    std::vector<MIRVtableSlot> slots; // VtableType
};

struct MIRDeclList {
    std::vector<MIRDeclItem> items;
    void clear() { items.clear(); }
};

} // namespace mir
