// Copyright (c) Chemical Language Foundation 2025.
//
// Shared interface-vtable layout (evaluation §1.4 / action B). The slot *order*
// and each slot's self-erased function type were previously derived
// independently by the C declaration emitter (`MIRDeclEmitter::vtable_slot_type`
// + its recursive walk) and the LLVM emitter (`mir_emit_llvm_module`'s
// `slots_of`/`slot_ftype`). Both now call this, so the layout exists once and a
// future JVM/JS backend gets it for free.
//
// AST-aware on purpose — a vtable slot is an interface method, and erasing its
// `self` to `void*` reads the method's return type, parameters and variadic
// flag. It is a front-end/declaration-collection component (like
// MIRDeclEmitter), not part of the AST-free MIR core, so it is built into the
// compiler (COMMON_SOURCES) but not into the standalone MIRTests target.

#pragma once

#include "MIRModule.h"

#include <string>
#include <vector>

class InterfaceDefinition;
class FunctionDeclaration;

namespace mir {

class MIRTypeBuilder;

/** One interface vtable slot: the method's name and its erased-self signature. */
struct MIRVtableSlot {
    std::string name;
    TypeId type = MIR_INVALID_ID;
};

/** The self-erased (`void* self`) function-signature type of one vtable method. */
TypeId mir_vtable_slot_type(MIRTypeBuilder& types, MIRModule& module,
                            FunctionDeclaration* func);

/**
 * The canonical, ordered slot list of `iface`: inherited interfaces first
 * (recursively, depth-first), then the interface's own `instantiated_functions()`.
 * `out_funcs`, when non-null, receives the matching declarations so a caller can
 * resolve a definition's callee (the C `create_vtable` and the LLVM impl path
 * both need the `FunctionDeclaration*`, not just its name/type).
 */
void mir_vtable_slots(MIRTypeBuilder& types, MIRModule& module, InterfaceDefinition* iface,
                      std::vector<MIRVtableSlot>& out,
                      std::vector<FunctionDeclaration*>* out_funcs = nullptr);

} // namespace mir
