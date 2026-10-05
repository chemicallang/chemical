// Copyright (c) Chemical Language Foundation 2025.

#include "MIRTypeBuilder.h"

#include "ast/base/BaseType.h"
#include "ast/base/BaseTypeKind.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionParam.h"

namespace mir {

namespace {

uint32_t int_bits_of(IntNTypeKind kind) {
    switch (kind) {
        case IntNTypeKind::Char:
        case IntNTypeKind::I8:
        case IntNTypeKind::UChar:
        case IntNTypeKind::U8:
            return 8;
        case IntNTypeKind::Short:
        case IntNTypeKind::I16:
        case IntNTypeKind::UShort:
        case IntNTypeKind::U16:
            return 16;
        case IntNTypeKind::Int:
        case IntNTypeKind::I32:
        case IntNTypeKind::UInt:
        case IntNTypeKind::U32:
            return 32;
        case IntNTypeKind::Long:
        case IntNTypeKind::LongLong:
        case IntNTypeKind::I64:
        case IntNTypeKind::ULong:
        case IntNTypeKind::ULongLong:
        case IntNTypeKind::U64:
            return 64;
        case IntNTypeKind::Int128:
        case IntNTypeKind::UInt128:
            return 128;
    }
    return 32;
}

} // namespace

TypeId MIRTypeBuilder::int_type(bool is_unsigned, uint32_t bits) {
    MIRTypeRecord r;
    r.kind = MIRTypeKind::Int;
    r.flags = static_cast<uint8_t>(is_unsigned ? 0 : TF_SIGNED);
    r.size = bits / 8;
    r.alignment = r.size;
    // distinguish signedness + width in the intern key via `decl` (unused for ints)
    r.decl = is_unsigned ? 1u : 0u;
    r.element = bits; // width participates in interning
    return intern(r);
}

TypeId MIRTypeBuilder::pointer_like(MIRTypeKind kind, TypeId child, bool is_mutable) {
    MIRTypeRecord r;
    r.kind = kind;
    r.flags = static_cast<uint8_t>(is_mutable ? TF_MUTABLE : TF_NONE);
    r.size = module_.layout.pointer_size;
    r.alignment = module_.layout.pointer_alignment;
    r.element = child;
    return intern(r);
}

TypeId MIRTypeBuilder::aggregate_type(MIRTypeKind kind, const void* decl, BaseType* type) {
    auto it = decl_types_.find(decl);
    if (it != decl_types_.end()) return it->second;

    uint32_t did = decl_ids_.size();
    decl_ids_[decl] = did;

    MIRTypeRecord r;
    r.kind = kind;
    r.decl = did;
    // size/alignment and field data are filled in later (aggregate milestone);
    // flags for ctor/dtor are available now.
    uint8_t flags = TF_NONE;
    if (type) {
        if (type->requires_destructor()) flags |= TF_HAS_DESTRUCTOR;
        if (type->get_def_constructor() != nullptr) flags |= TF_HAS_CONSTRUCTOR;
        if (type->requires_moving()) flags |= TF_NONE; // moves tracked via contract later
    }
    r.flags = flags;

    TypeId id = static_cast<TypeId>(module_.types.types.size());
    module_.types.types.push_back(r);
    decl_types_[decl] = id;
    return id;
}

TypeId MIRTypeBuilder::function_type(BaseType* type) {
    auto* ft = type->as_function_type();
    if (!ft) ft = type->get_canonical_function_type();
    if (!ft) return opaque_type();

    MIRTypeRecord r;
    r.kind = MIRTypeKind::Function;
    r.size = module_.layout.pointer_size;
    r.alignment = module_.layout.pointer_alignment;
    r.element = ft->returnType ? map(ft->returnType) : void_type();

    uint32_t n = static_cast<uint32_t>(ft->params.size());
    // params are mapped first into a temporary to avoid re-entrancy issues
    std::vector<TypeId> param_ids;
    param_ids.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        FunctionParam* p = ft->params[i];
        param_ids.push_back(p && p->type ? map(p->type) : opaque_type());
    }
    if (n) r.data_offset = module_.types.append_data(param_ids.data(), n);
    r.data_count = n;
    return intern(r);
}

TypeId MIRTypeBuilder::pointer_type(TypeId pointee, bool is_mutable) {
    return pointer_like(MIRTypeKind::Pointer, pointee, is_mutable);
}

TypeId MIRTypeBuilder::function_signature(TypeId return_type, const std::vector<TypeId>& params) {
    MIRTypeRecord r;
    r.kind = MIRTypeKind::Function;
    r.size = module_.layout.pointer_size;
    r.alignment = module_.layout.pointer_alignment;
    r.element = return_type;
    uint32_t n = static_cast<uint32_t>(params.size());
    if (n) r.data_offset = module_.types.append_data(params.data(), n);
    r.data_count = n;
    return intern(r);
}

TypeId MIRTypeBuilder::map(BaseType* type) {
    if (!type) return opaque_type();

    auto it = memo_.find(type);
    if (it != memo_.end()) return it->second;

    TypeId id = opaque_type();
    switch (type->kind()) {
        case BaseTypeKind::Void:
            id = void_type();
            break;
        case BaseTypeKind::Bool:
            id = bool_type();
            break;
        case BaseTypeKind::IntN: {
            auto* in = type->as_intn_type();
            id = int_type(in->is_unsigned(), int_bits_of(in->IntNKind()));
            break;
        }
        case BaseTypeKind::Float: {
            id = intern_simple(MIRTypeKind::Float, 4, 4);
            break;
        }
        case BaseTypeKind::Double: {
            id = intern_simple(MIRTypeKind::Float, 8, 8);
            break;
        }
        case BaseTypeKind::LongDouble:
        case BaseTypeKind::Float128: {
            id = intern_simple(MIRTypeKind::Float, 16, 16);
            break;
        }
        case BaseTypeKind::Pointer: {
            auto* pt = type->as_pointer_type();
            id = pointer_like(MIRTypeKind::Pointer, map(pt->type), pt->is_mutable);
            break;
        }
        case BaseTypeKind::Reference: {
            auto* rt = type->as_reference_type();
            id = pointer_like(MIRTypeKind::Reference, map(rt->type), rt->is_mutable);
            break;
        }
        case BaseTypeKind::String: {
            // string surfaces as a pointer-like handle at the MIR level
            id = pointer_like(MIRTypeKind::Pointer, void_type(), false);
            break;
        }
        case BaseTypeKind::Array: {
            auto* at = type->as_array_type();
            MIRTypeRecord r;
            r.kind = MIRTypeKind::Array;
            r.element = map(at->known_child_type());
            uint64_t len = at->get_array_size();
            const MIRTypeRecord& elem = module_.types.get(r.element);
            r.size = static_cast<uint32_t>(elem.size * len);
            r.alignment = elem.alignment;
            r.data_count = static_cast<uint32_t>(len);
            id = intern(r);
            break;
        }
        case BaseTypeKind::Struct: {
            id = aggregate_type(MIRTypeKind::Struct, type->get_direct_linked_struct(), type);
            break;
        }
        case BaseTypeKind::Union: {
            id = aggregate_type(MIRTypeKind::Union, type->get_direct_linked_node(), type);
            break;
        }
        case BaseTypeKind::Linked: {
            if (auto* v = type->get_direct_linked_variant()) {
                id = aggregate_type(MIRTypeKind::Variant, v, type);
            } else if (auto* e = type->get_direct_linked_enum()) {
                id = aggregate_type(MIRTypeKind::Int, e, type);
            } else if (auto* s = type->get_direct_linked_struct()) {
                id = aggregate_type(MIRTypeKind::Struct, s, type);
            } else {
                id = opaque_type();
            }
            break;
        }
        case BaseTypeKind::Function:
        case BaseTypeKind::CapturingFunction:
            id = function_type(type);
            break;
        default:
            id = opaque_type();
            break;
    }

    memo_[type] = id;
    return id;
}

} // namespace mir
