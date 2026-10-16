// Copyright (c) Chemical Language Foundation 2025.
//
// Shared interface-vtable layout and declaration records (evaluation §1.4 /
// action B; A3's interface residual). The slot *order*, each slot's self-erased
// function type, and the resolved callee are computed once here:
//
//   * `mir_vtable_slot_type` / `mir_vtable_slots` — the layout, consumed by the
//     C declaration pass (`MIRDeclEmitter`);
//   * `build_module_vtables` — fills `MIRModule::vtables` with the type and
//     definition records a backend can emit without touching the AST, consumed
//     by the LLVM driver (`mir_emit_llvm_module`).
//
// AST-aware on purpose — a vtable slot is an interface method, and erasing its
// `self` to `void*` reads the method's return type, parameters and variadic
// flag. It is a front-end/declaration-collection component (like
// MIRDeclEmitter), not part of the AST-free MIR core, so it is built into the
// compiler (COMMON_SOURCES) but not into the standalone MIRTests target.

#pragma once

#include "MIRModule.h"
#include "MIRVtableTable.h"

#include <vector>

class ASTNode;
class InterfaceDefinition;
class FunctionDeclaration;
class NameMangler;

namespace mir {

class MIRTypeBuilder;

/** The self-erased (`void* self`) function-signature type of one vtable method. */
TypeId mir_vtable_slot_type(MIRTypeBuilder& types, MIRModule& module,
                            FunctionDeclaration* func);

/**
 * The canonical, ordered slot list of `iface`: inherited interfaces first
 * (recursively, depth-first), then the interface's own `instantiated_functions()`.
 * `out_funcs`, when non-null, receives the matching declarations so a caller can
 * resolve a definition's callee.
 */
void mir_vtable_slots(MIRTypeBuilder& types, MIRModule& module, InterfaceDefinition* iface,
                      std::vector<MIRVtableSlot>& out,
                      std::vector<FunctionDeclaration*>* out_funcs = nullptr);

/**
 * Build the module's vtable type and definition records (`module.vtables`) from
 * its top-level declarations, so a backend can emit vtables from MIR alone. The
 * callee names are resolved here (mangling, the interface's `active_user`), which
 * is what lets `mir_emit_llvm_module` stop walking the AST.
 */
void build_module_vtables(NameMangler& mangler, MIRTypeBuilder& types, MIRModule& module,
                          const std::vector<ASTNode*>& nodes, bool clear = true);

} // namespace mir
