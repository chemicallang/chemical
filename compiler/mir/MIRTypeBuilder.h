// Copyright (c) Chemical Language Foundation 2025.
//
// Maps resolved AST `BaseType`s to canonical MIR `TypeId`s. This is the one
// MIR component allowed to depend on AST types (it is the lowering boundary);
// the core MIR headers stay AST-free. See mir-implementation-plan.md §2.6 and
// §4 Stage 2.5.

#pragma once

#include "MIRModule.h"

#include <unordered_map>

class BaseType;

namespace mir {

class MIRTypeBuilder {
public:
    explicit MIRTypeBuilder(MIRModule& module) : module_(module) {}

    /** Map an AST type to a canonical MIR type id. Never returns INVALID. */
    TypeId map(BaseType* type);

    /** Build a function type from a return type and parameter type ids. */
    TypeId function_signature(TypeId return_type, const std::vector<TypeId>& params);

    /** The module type table this maps into. */
    MIRModule& module() { return module_; }

    TypeId void_type() { return intern_simple(MIRTypeKind::Void, 0, 1); }
    TypeId opaque_type() { return intern_simple(MIRTypeKind::Opaque, 0, 1); }
    TypeId bool_type() {
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Bool;
        r.size = 1;
        r.alignment = 1;
        return intern(r);
    }

private:
    TypeId intern(const MIRTypeRecord& rec) { return module_.types.intern(rec); }

    TypeId intern_simple(MIRTypeKind kind, uint32_t size, uint32_t align) {
        MIRTypeRecord r;
        r.kind = kind;
        r.size = size;
        r.alignment = align;
        return intern(r);
    }

    TypeId int_type(bool is_unsigned, uint32_t bits);
    TypeId pointer_like(MIRTypeKind kind, TypeId child, bool is_mutable);
    TypeId aggregate_type(MIRTypeKind kind, const void* decl, BaseType* type);
    TypeId function_type(BaseType* type);

    MIRModule& module_;
    std::unordered_map<const BaseType*, TypeId> memo_;
    std::unordered_map<const void*, uint32_t> decl_ids_;
    std::unordered_map<const void*, TypeId> decl_types_;
};

} // namespace mir
