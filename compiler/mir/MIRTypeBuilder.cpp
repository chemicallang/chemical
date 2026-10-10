// Copyright (c) Chemical Language Foundation 2025.

#include "MIRTypeBuilder.h"

#include "ast/base/BaseType.h"
#include "ast/base/BaseTypeKind.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/FunctionType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/StructType.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/GenericType.h"
#include "ast/types/LiteralType.h"
#include "ast/types/MaybeRuntimeType.h"
#include "ast/types/RuntimeType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/BaseDefMember.h"
#include "ast/structures/VariablesContainer.h"

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

MIRIntKind mir_int_kind(IntNTypeKind kind) {
    switch (kind) {
        case IntNTypeKind::I8: return IK_I8;
        case IntNTypeKind::I16: return IK_I16;
        case IntNTypeKind::I32: return IK_I32;
        case IntNTypeKind::I64: return IK_I64;
        case IntNTypeKind::Int128: return IK_I128;
        case IntNTypeKind::U8: return IK_U8;
        case IntNTypeKind::U16: return IK_U16;
        case IntNTypeKind::U32: return IK_U32;
        case IntNTypeKind::U64: return IK_U64;
        case IntNTypeKind::UInt128: return IK_U128;
        case IntNTypeKind::Char: return IK_CHAR;
        case IntNTypeKind::Short: return IK_SHORT;
        case IntNTypeKind::Int: return IK_INT;
        case IntNTypeKind::Long: return IK_LONG;
        case IntNTypeKind::LongLong: return IK_LONGLONG;
        case IntNTypeKind::UChar: return IK_UCHAR;
        case IntNTypeKind::UShort: return IK_USHORT;
        case IntNTypeKind::UInt: return IK_UINT;
        case IntNTypeKind::ULong: return IK_ULONG;
        case IntNTypeKind::ULongLong: return IK_ULONGLONG;
    }
    return IK_UNSPECIFIED;
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

void MIRTypeBuilder::ensure_inline_struct_name(TypeId id, BaseType* struct_type) {
    if (id == MIR_INVALID_ID || !struct_type) return;
    if (!module_.types.name_of(id).empty()) return;
    const chem::string_view sv = struct_type->as_struct_type_unsafe()->name;
    std::string name;
    if (!sv.empty()) {
        name.assign(sv.data(), sv.size());
    } else {
        name = "__chx_inline_" + std::to_string(inline_counter_++);
    }
    module_.types.set_name(id, name.data(), static_cast<uint32_t>(name.size()));
}

TypeId MIRTypeBuilder::aggregate_type(MIRTypeKind kind, const void* decl, BaseType* type) {
    const auto agg_key = std::make_pair(decl, static_cast<uint8_t>(kind));
    auto it = decl_types_.find(agg_key);
    if (it != decl_types_.end()) return it->second;

    uint32_t did = static_cast<uint32_t>(decl_ids_.size());
    decl_ids_[agg_key] = did;

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

    if (kind == MIRTypeKind::Int) {
        // enum types are spelled `int` by the legacy backend
        r.size = 4;
        r.alignment = 4;
        r.flags = TF_SIGNED;
        r.int_kind = IK_INT;
    }

    TypeId id = static_cast<TypeId>(module_.types.types.size());
    module_.types.types.push_back(r);
    decl_types_[agg_key] = id;
    if (name_resolver_ && decl) {
        std::string name = name_resolver_(reinterpret_cast<ASTNode*>(const_cast<void*>(decl)));
        if (!name.empty()) {
            module_.types.set_name(id, name.data(), static_cast<uint32_t>(name.size()));
        }
    }
    fill_aggregate_fields(id, kind, reinterpret_cast<ASTNode*>(const_cast<void*>(decl)), type);
    return id;
}

void MIRTypeBuilder::fill_aggregate_fields(TypeId id, MIRTypeKind kind, ASTNode* decl,
                                           BaseType* type) {
    if (kind == MIRTypeKind::Int) return; // enums have no fields
    if (id >= module_.types.types.size()) return;

    std::vector<MIRFieldInfo> fs;
    auto push_field = [&](const std::string& name, TypeId t, MIRFieldKind fk) {
        MIRFieldInfo fi;
        fi.type = t;
        fi.kind = static_cast<uint8_t>(fk);
        fi.index = static_cast<uint32_t>(fs.size());
        fi.name_offset = static_cast<uint32_t>(module_.types.field_names.size());
        fi.name_length = static_cast<uint32_t>(name.size());
        module_.types.field_names.insert(module_.types.field_names.end(), name.begin(), name.end());
        fs.push_back(fi);
    };
    auto add_bases = [&](VariablesContainer* vc) {
        if (!vc) return;
        for (auto& inh : vc->inherited) {
            StructDefinition* base = inh.type ? inh.type->get_direct_linked_struct() : nullptr;
            if (!base) continue;
            TypeId bt = map(base->known_type());
            const chem::string_view nv = base->name_view();
            push_field(std::string(nv.data(), nv.size()), bt, MIRFieldKind::Base);
        }
    };
    auto add_members = [&](VariablesContainerBase* vc) {
        if (!vc) return;
        for (BaseDefMember* var : vc->variables()) {
            if (!var) continue;
            TypeId vt = var->known_type() ? map(var->known_type()) : opaque_type();
            const chem::string_view nv = var->name_view();
            push_field(std::string(nv.data(), nv.size()), vt, MIRFieldKind::Direct);
        }
    };

    VariablesContainer* vc = nullptr;
    VariablesContainerBase* vcb = nullptr;
    if (decl) {
        switch (decl->kind()) {
            case ASTNodeKind::StructDecl: vc = decl->as_struct_def_unsafe(); break;
            case ASTNodeKind::UnionDecl: vc = decl->as_union_def_unsafe(); break;
            case ASTNodeKind::UnnamedStruct: vcb = decl->as_unnamed_struct_unsafe(); break;
            case ASTNodeKind::UnnamedUnion: vcb = decl->as_unnamed_union_unsafe(); break;
            case ASTNodeKind::StructType: vc = static_cast<StructType*>(decl); break;
            default: break;
        }
    }
    if (!vc && !vcb && type) {
        switch (type->kind()) {
            case BaseTypeKind::Struct: vc = type->as_struct_type_unsafe(); break;
            default: break;
        }
    }

    if (decl && decl->kind() == ASTNodeKind::VariantDecl) {
        auto* vd = decl->as_variant_def_unsafe();
        add_bases(vd);
        // discriminator (`__chx__vt_621827`), always `int` (i32)
        push_field("__chx__vt_621827", int_type(false, 32), MIRFieldKind::VariantTag);
        // one payload struct per case (the emitter lays out the largest and
        // bitcasts to a specific case)
        for (BaseDefMember* var : vd->variables()) {
            auto* member = var ? var->as_variant_member_unsafe() : nullptr;
            if (!member) continue;
            // build the case payload struct type
            std::vector<MIRFieldInfo> case_fields;
            uint32_t ci = 0;
            for (auto& entry : member->values) {
                VariantMemberParam* param = entry.second;
                if (!param) continue;
                MIRFieldInfo fi;
                fi.type = param->type ? map(param->type) : opaque_type();
                fi.kind = static_cast<uint8_t>(MIRFieldKind::VariantParam);
                fi.index = ci++;
                const chem::string_view pn = param->name;
                fi.name_offset = static_cast<uint32_t>(module_.types.field_names.size());
                fi.name_length = static_cast<uint32_t>(pn.size());
                module_.types.field_names.insert(module_.types.field_names.end(), pn.data(),
                                                 pn.data() + pn.size());
                case_fields.push_back(fi);
            }
            MIRTypeRecord cr;
            cr.kind = MIRTypeKind::Struct;
            cr.data_count = static_cast<uint32_t>(case_fields.size());
            cr.field_index = module_.types.append_fields(case_fields);
            TypeId case_id = static_cast<TypeId>(module_.types.types.size());
            module_.types.types.push_back(cr);
            const chem::string_view mn = member->name_view();
            std::string cname = "__chx_case_" + std::to_string(id) + "_" +
                                std::string(mn.data(), mn.size());
            module_.types.set_name(case_id, cname.data(), static_cast<uint32_t>(cname.size()));
            push_field(std::string(mn.data(), mn.size()), case_id, MIRFieldKind::VariantCase);
        }
    } else if (vc) {
        add_bases(vc);
        add_members(vc);
    } else if (vcb) {
        add_members(vcb);
    }

    MIRTypeRecord& rec = module_.types.types[id];
    rec.data_count = static_cast<uint32_t>(fs.size());
    rec.field_index = fs.empty() ? MIR_INVALID_ID : module_.types.append_fields(fs);
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
    // variadic: the last parameter is the tail marker and is spelled as `...`
    if (ft->isVariadic() && n > 0) {
        n -= 1;
        r.flags |= TF_VARIADIC;
    }
    // params are mapped first into a temporary to avoid re-entrancy issues
    std::vector<TypeId> param_ids;
    param_ids.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        FunctionParam* p = ft->params[i];
        TypeId pid = p && p->type ? map(p->type) : opaque_type();
        // aggregate/array parameters are passed by pointer, matching the
        // function definitions emitted for normal functions
        if (pid != MIR_INVALID_ID && pid < module_.types.size()) {
            const MIRTypeRecord& pr = module_.types.get(pid);
            if (!(pr.flags & TF_TYPEDEF)) {
                if (pr.kind == MIRTypeKind::Array) {
                    pid = pointer_type(pr.element, true);
                } else if (pr.kind == MIRTypeKind::Struct || pr.kind == MIRTypeKind::Union ||
                           pr.kind == MIRTypeKind::Variant) {
                    pid = pointer_type(pid, true);
                }
            }
        }
        param_ids.push_back(pid);
    }
    if (n) r.data_offset = module_.types.append_data(param_ids.data(), n);
    r.data_count = n;
    return intern(r);
}

TypeId MIRTypeBuilder::pointer_type(TypeId pointee, bool is_mutable) {
    return pointer_like(MIRTypeKind::Pointer, pointee, is_mutable);
}

TypeId MIRTypeBuilder::function_signature(TypeId return_type, const std::vector<TypeId>& params,
                                          bool variadic) {
    MIRTypeRecord r;
    r.kind = MIRTypeKind::Function;
    r.size = module_.layout.pointer_size;
    r.alignment = module_.layout.pointer_alignment;
    r.element = return_type;
    if (variadic) r.flags |= TF_VARIADIC;
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
            const IntNTypeKind k = in->IntNKind();
            MIRTypeRecord r;
            r.kind = MIRTypeKind::Int;
            r.flags = static_cast<uint8_t>(in->is_unsigned() ? 0 : TF_SIGNED);
            r.size = int_bits_of(k) / 8;
            r.alignment = r.size;
            r.decl = in->is_unsigned() ? 1u : 0u;
            r.element = int_bits_of(k);
            r.int_kind = mir_int_kind(k);
            id = intern(r);
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
            TypeId pointee = map(pt->type);
            // NOTE: do NOT decay an array pointee to a pointer here. `*mut [N]T`
            // is a genuine pointer-to-array (`T (*)[N]`); decaying it to `T**`
            // breaks indexing/stride through the pointer (`&raw mut a[i]` on a
            // multi-dimensional array). The declarator/c_type_of spellings handle
            // pointer-to-array directly.
            id = pointer_like(MIRTypeKind::Pointer, pointee, pt->is_mutable);
            break;
        }
        case BaseTypeKind::Reference: {
            auto* rt = type->as_reference_type();
            id = pointer_like(MIRTypeKind::Reference, map(rt->type), rt->is_mutable);
            break;
        }
        case BaseTypeKind::String: {
            // string literals are `char*` (matches the legacy backend)
            id = pointer_type(char_type(), true);
            break;
        }
        case BaseTypeKind::Dynamic: {
            id = fat_pointer_type();
            break;
        }
        case BaseTypeKind::Literal: {
            auto* lt = type->as_literal_type();
            id = (lt && lt->underlying) ? map(lt->underlying) : opaque_type();
            break;
        }
        case BaseTypeKind::MaybeRuntime: {
            auto* mr = type->as_maybe_runtime_type_unsafe();
            id = (mr && mr->underlying) ? map(mr->underlying) : opaque_type();
            break;
        }
        case BaseTypeKind::Runtime: {
            auto* rt = type->as_runtime_type_unsafe();
            id = (rt && rt->underlying) ? map(rt->underlying) : opaque_type();
            break;
        }
        case BaseTypeKind::Array: {
            auto* at = type->as_array_type();
            if (!at->has_array_size()) {
                // a dynamic array (`[]T`) decays to a pointer to its element
                id = pointer_type(map(at->known_child_type()), true);
                break;
            }
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
            const void* decl = type->get_direct_linked_struct();
            if (!decl) decl = type->get_direct_linked_node();
            id = aggregate_type(MIRTypeKind::Struct, decl, type);
            ensure_inline_struct_name(id, type);
            module_.record_inline_struct(id, type);
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
            } else if (ASTNode* n = type->get_direct_linked_node()) {
                if (n->kind() == ASTNodeKind::UnionDecl) {
                    id = aggregate_type(MIRTypeKind::Union, n, type);
                } else if (n->kind() == ASTNodeKind::UnnamedUnion) {
                    id = aggregate_type(MIRTypeKind::Union, n, type);
                } else if (n->kind() == ASTNodeKind::UnnamedStruct) {
                    id = aggregate_type(MIRTypeKind::Struct, n, type);
                } else if (n->kind() == ASTNodeKind::StructType) {
                    id = aggregate_type(MIRTypeKind::Struct, n, type);
                    ensure_inline_struct_name(id, static_cast<StructType*>(n));
                    module_.record_inline_struct(id, n);
                } else if (n->kind() == ASTNodeKind::TypealiasStmt) {
                    id = map(n->as_typealias_unsafe()->known_type());
                } else if (n->kind() == ASTNodeKind::FunctionDecl) {
                    BaseType* ft = n->as_function_unsafe()->known_type();
                    id = ft ? map(ft) : opaque_type();
                } else {
                    id = opaque_type();
                }
            } else {
                id = opaque_type();
            }
            break;
        }
        case BaseTypeKind::Generic: {
            if (auto* v = type->get_direct_linked_variant()) {
                id = aggregate_type(MIRTypeKind::Variant, v, type);
            } else if (auto* e = type->get_direct_linked_enum()) {
                id = aggregate_type(MIRTypeKind::Int, e, type);
            } else if (auto* s = type->get_direct_linked_struct()) {
                id = aggregate_type(MIRTypeKind::Struct, s, type);
            } else if (auto* g = type->as_generic_type()) {
                if (g->referenced) id = map(g->referenced);
                else id = opaque_type();
            } else if (ASTNode* n = type->get_direct_linked_node()) {
                if (n->kind() == ASTNodeKind::TypealiasStmt) {
                    id = map(n->as_typealias_unsafe()->known_type());
                } else {
                    id = opaque_type();
                }
            } else {
                id = opaque_type();
            }
            break;
        }
        case BaseTypeKind::Any:
            // `any` is spelled `void` (so `*mut any` is `void*`)
            id = void_type();
            break;
        case BaseTypeKind::Function:
            id = function_type(type);
            break;
        case BaseTypeKind::CapturingFunction:
            // a capturing function type is spelled as its instance struct
            // (matching the legacy `VisitCapturingFunctionType`), not a raw
            // function pointer
            id = map(type->as_capturing_func_type_unsafe()->instance_type);
            break;
        default:
            id = opaque_type();
            break;
    }

    memo_[type] = id;
    return id;
}

} // namespace mir
