// Copyright (c) Chemical Language Foundation 2025.
//
// Shared global-declaration spelling (action A3, globals). `declare_var` (the
// declaration) and `implement_global` (the definition) each computed the same
// name, declared type — including the unsized-`T[]` declarator — and
// thread-local/static/volatile flags from the `VarInitStatement`. They now share
// `build_global_declaration` and the `MIRGlobalDecl` record; the definition adds
// the initializer (`MIRDeclPrinter::const_expr_c` over a `MIRConstExpr`).
//
// AST-free record (ids + name only) so MIRModule can include it; the builder is
// AST-aware and lives in COMMON_SOURCES.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
#include <string>

class VarInitStatement;
class NameMangler;

namespace mir {

struct MIRModule;
class MIRTypeBuilder;

enum MIRGlobalDeclFlags : uint8_t {
    MGD_NONE = 0x00,
    MGD_THREAD_LOCAL = 0x01,
    MGD_VOLATILE = 0x02,
    MGD_STATIC = 0x04,
};

/** A top-level global's declaration: name, declared type, linkage flags. */
struct MIRGlobalDecl {
    std::string name;
    /** The mapped declared type; `MIR_INVALID_ID` when the global has no spelled
     *  type (a bare `void` declaration). */
    TypeId type = MIR_INVALID_ID;
    /** When `unsized_array`, the mapped element type (the declarator is `T name[]`). */
    TypeId elem = MIR_INVALID_ID;
    bool unsized_array = false;
    uint8_t flags = MGD_NONE;
};

/** Fill `out` with `init`'s declaration (name, type, unsized-array, flags). */
void build_global_declaration(NameMangler& mangler, MIRTypeBuilder& types, MIRModule& module,
                              VarInitStatement* init, MIRGlobalDecl& out);

/**
 * The name-agnostic half of `build_global_declaration`: the declared type (or
 * unsized-`T[]` element), the unsized-array flag and the TLS/static/volatile
 * flags, leaving `out.name` untouched. A caller that already has a mangled name
 * (the lowerer, which mangles through its locked callback rather than a
 * `NameMangler&`) fills the name itself.
 */
void build_global_declaration_type(MIRTypeBuilder& types, VarInitStatement* init,
                                   MIRGlobalDecl& out);

} // namespace mir
