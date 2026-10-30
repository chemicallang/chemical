// Copyright (c) Chemical Language Foundation 2025.
//
// Shared function-prototype ABI and record (action A3, prototypes). The
// self-erased/aggregate/sret function signature of a source `FunctionDeclaration`
// was built twice: `MIRDeclEmitter::function_type_of` (the C prototype) and the
// signature block in `MIRLowerer::intern_function` (the LLVM symbol/call). Both
// carry a comment saying they must match. `mir_function_signature` is now the one
// implementation, and `MIRPrototype` is the record a declaration printer
// consumes (name, signature type, extern/static/dllimport flags).
//
// AST-aware (a prototype is a source function), so it is built into the compiler
// (COMMON_SOURCES) but not into the standalone MIRTests target.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
#include <string>

class FunctionDeclaration;
class NameMangler;

namespace mir {

struct MIRModule;
class MIRTypeBuilder;

enum MIRPrototypeFlags : uint8_t {
    MPF_NONE = 0x00,
    MPF_EXTERN = 0x01,
    MPF_STATIC = 0x02,
    MPF_DLLIMPORT = 0x04,
};

/** A function declaration's C prototype: mangled name, signature, linkage flags. */
struct MIRPrototype {
    std::string name; // mangled, including any `__chem_stdcall` prefix
    TypeId type = MIR_INVALID_ID;
    // Set for an `@extern` function whose C ABI differs from the internal ABI.
    TypeId c_abi_type = MIR_INVALID_ID;
    uint8_t flags = MPF_NONE;
};

/** The ABI function-signature `TypeId` of `decl` (sret, aggregates/arrays by
 *  pointer, variadic tail marker dropped, static-interface self erased). */
TypeId mir_function_signature(MIRTypeBuilder& types, MIRModule& module,
                              FunctionDeclaration* decl);

/** The by-value C ABI signature of `decl` (no hidden sret/pointer conversion). */
TypeId mir_c_function_signature(MIRTypeBuilder& types, MIRModule& module,
                                FunctionDeclaration* decl);

/** True when an extern declaration has a by-value struct/union parameter or
 *  return, so its C signature differs from the internal prototype ABI. */
bool mir_needs_c_abi(FunctionDeclaration* decl);

/** Fill `out` with `decl`'s prototype (name + flags + signature). */
void build_function_prototype(NameMangler& mangler, MIRTypeBuilder& types, MIRModule& module,
                              FunctionDeclaration* decl, MIRPrototype& out);

} // namespace mir
