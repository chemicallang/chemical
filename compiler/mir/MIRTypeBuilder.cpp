// Copyright (c) Chemical Language Foundation 2025.

#include "MIRTypeBuilder.h"
#include "MIRHotOps.h"

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

#include <string_view>

namespace mir {

namespace {

// `long`/`ulong` are target-dependent: 64-bit on 64-bit POSIX, but 32-bit on
// Windows (LLP64). Mirrors `IntNType::num_bits(TargetData)`.
uint32_t int_bits_of(IntNTypeKind kind, bool is64Bit, bool windows) {
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
        case IntNTypeKind::ULong:
            return (is64Bit && !windows) ? 64 : 32;
        case IntNTypeKind::LongLong:
        case IntNTypeKind::I64:
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
    if (module_.types.has_name(id)) return;
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
    // Keyed by (declaration node, aggregate kind): the AST arena reuses addresses
    // after a file allocator reset, so keying on the bare pointer would collide a
    // struct and a variant that happened to land on the same address. One flat
    // map per kind does the same job as the single `std::map` keyed on the pair
    // (the kind is a small dense enum), without the tree walk `aggregate_type`
    // paid on every one of its calls.
    const uint32_t kind_slot = static_cast<uint32_t>(kind) & (kDeclKindSlots - 1u);
    const uint64_t decl_key = reinterpret_cast<uintptr_t>(decl);
    const uint32_t cached = decl_types_[kind_slot].find(decl_key);
    if (cached != MIRKeyMap::kNotFound) return cached;

    const uint32_t did = next_decl_id_++;

    MIRTypeRecord r;
    r.kind = kind;
    r.decl = did;
    // size/alignment/field offsets are filled in by
    // `fill_aggregate_fields` (below, through `compute_aggregate_layout`);
    // flags for ctor/dtor are available now.
    uint8_t flags = TF_NONE;
    if (type) {
        if (type->requires_destructor()) flags |= TF_HAS_DESTRUCTOR;
        if (type->has_user_destructor()) flags |= TF_USER_DESTRUCTOR;
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

    TypeId id = module_.types.push(r);
    decl_types_[kind_slot].set(decl_key, id);
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
    if (id >= module_.types.size()) return;

    // The field count is known for the member path, so `fs` grows once instead
    // of through a handful of `push_back` reallocations per aggregate. (A
    // variant's count is the tag + its cases + the bases, hence the slack.)
    std::vector<MIRFieldInfo> fs;
    {
        size_t hint = 0;
        if (auto* vc = decl ? decl->as_extendable_member_container() : nullptr) {
            hint = vc->variables().size() + 2;
        }
        fs.reserve(hint ? hint : 4);
    }
    // `std::string_view`, not `std::string`: every caller already holds the
    // AST's name bytes, and constructing a `std::string` per field only to hand
    // its bytes straight to the pool allocated for nothing.
    auto push_field = [&](std::string_view name, TypeId t, MIRFieldKind fk,
                          uint32_t explicit_align = 0, uint8_t spelling_flags = MFF_NONE) {
        MIRFieldInfo fi;
        fi.type = t;
        fi.kind = static_cast<uint8_t>(fk);
        fi.flags = spelling_flags;
        fi.index = static_cast<uint32_t>(fs.size());
        fi.align = explicit_align;
        fi.name_offset = module_.types.append_field_name(name.data(), static_cast<uint32_t>(name.size()));
        fi.name_length = static_cast<uint32_t>(name.size());
        fs.push_back(fi);
    };
    auto add_bases = [&](VariablesContainer* vc) {
        if (!vc) return;
        for (auto& inh : vc->inherited) {
            StructDefinition* base = inh.type ? inh.type->get_direct_linked_struct() : nullptr;
            if (!base) continue;
            TypeId bt = map(base->known_type());
            const chem::string_view nv = base->name_view();
            push_field(nv.view(), bt, MIRFieldKind::Base);
        }
    };
    auto add_members = [&](VariablesContainerBase* vc) {
        if (!vc) return;
        for (BaseDefMember* var : vc->variables()) {
            if (!var) continue;
            TypeId vt = var->known_type() ? map(var->known_type()) : opaque_type();
            const chem::string_view nv = var->name_view();
            // an explicit `@align`/`__chx_align(N)` on the member raises its
            // alignment (the emitter writes the attribute out verbatim)
            StructMember* sm = var->as_struct_member();
            const uint32_t req = sm ? sm->get_required_alignment() : 0;
            // spelling facts the member carries but layout does not, so the C
            // declaration printer can spell the field from MIR alone (action A3)
            uint8_t sflags = MFF_NONE;
            if (sm && sm->is_volatile()) sflags |= MFF_VOLATILE;
            BaseType* mt = var->known_type();
            if (mt && mt->kind() == BaseTypeKind::CapturingFunction) sflags |= MFF_FAT_POINTER;
            switch (var->kind()) {
                case ASTNodeKind::UnnamedStruct: sflags |= MFF_ANONYMOUS_STRUCT; break;
                case ASTNodeKind::UnnamedUnion: sflags |= MFF_ANONYMOUS_UNION; break;
                default: break;
            }
            push_field(nv.view(), vt, MIRFieldKind::Direct, req, sflags);
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
                fi.name_offset = module_.types.append_field_name(pn.data(), static_cast<uint32_t>(pn.size()));
                fi.name_length = static_cast<uint32_t>(pn.size());
                case_fields.push_back(fi);
            }
            MIRTypeRecord cr;
            cr.kind = MIRTypeKind::Struct;
            cr.data_count = static_cast<uint32_t>(case_fields.size());
            cr.field_index = module_.types.append_fields(case_fields);
            const TypeId case_id = module_.types.push(cr);
            // the case payload is an aggregate too: give it its layout
            compute_aggregate_layout(module_.types, case_id);
            const chem::string_view mn = member->name_view();
            // one reused buffer: a variant case name is the only concat here
            static thread_local std::string cname;
            cname.assign("__chx_case_");
            cname += std::to_string(id);
            cname += '_';
            cname.append(mn.data(), mn.size());
            module_.types.set_name(case_id, cname.data(), static_cast<uint32_t>(cname.size()));
            push_field(mn.view(), case_id, MIRFieldKind::VariantCase);
        }
    } else if (vc) {
        add_bases(vc);
        add_members(vc);
    } else if (vcb) {
        add_members(vcb);
    }

    const uint32_t fidx = fs.empty() ? MIR_INVALID_ID : module_.types.append_fields(fs);
    // size, alignment and each field's byte offset, from the canonical table
    // alone (never from the host or from the AST again)
    uint32_t explicit_align = 0;
    if (decl && decl->kind() == ASTNodeKind::StructDecl) {
        explicit_align = decl->as_struct_def_unsafe()->get_required_alignment();
    }
    module_.types.update(id, [&](MIRTypeRecord& rec) {
        rec.data_count = static_cast<uint32_t>(fs.size());
        rec.field_index = fidx;
        rec.explicit_alignment = explicit_align;
    });
    compute_aggregate_layout(module_.types, id, explicit_align);
}

void MIRTypeBuilder::finalize_layouts() {
    const uint32_t count = module_.types.size();
    if (count == 0) return;

    // A by-value chain has at most `count` links, so this bounds the fixpoint.
    for (uint32_t pass = 0; pass <= count; ++pass) {
        bool changed = false;

        // Arrays derive their extent from the element, which may itself be a
        // multi-dimensional array or a recursive aggregate.
        for (TypeId id = 0; id < count; ++id) {
            MIRTypeRecord& rec = module_.types.types[id];
            if (rec.kind != MIRTypeKind::Array || rec.data_count == 0) continue;
            if (rec.element >= count) continue;
            const MIRTypeRecord& elem = module_.types.types[rec.element];
            const uint32_t size = elem.size * rec.data_count;
            if (size != rec.size || elem.alignment != rec.alignment) {
                rec.size = size;
                rec.alignment = elem.alignment;
                changed = true;
            }
        }

        for (TypeId id = 0; id < count; ++id) {
            const MIRTypeRecord before = module_.types.types[id];
            if (before.kind != MIRTypeKind::Struct && before.kind != MIRTypeKind::Union &&
                before.kind != MIRTypeKind::Variant) {
                continue;
            }
            if (before.field_index == MIR_INVALID_ID) continue;
            compute_aggregate_layout(module_.types, id, before.explicit_alignment);
            const MIRTypeRecord& after = module_.types.types[id];
            if (after.size != before.size || after.alignment != before.alignment) {
                changed = true;
            }
        }

        if (!changed) break;
    }
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
    // The memo/declaration caches are only raced by parallel lowering; the
    // shipping pipeline is single-writer, so the recursive mutex is elided there
    // (see MIRTableLock.h). Recursion inside `map` re-enters this same thread.
    const bool multi = !module_.single_threaded();
    MIR_OP(type_map);
    // Lock-free fast path: a per-thread memo of already-mapped types. Only a
    // genuine miss (a type no other worker mapped under this epoch) takes the
    // builder lock.
    ThreadMemo* tm = nullptr;
    if (multi) {
        tm = &thread_memo();
        const uint64_t e = epoch_.load(std::memory_order_relaxed);
        if (tm->epoch != e) {
            tm->epoch = e;
            tm->memo.clear();
        }
        const uint32_t hit = tm->memo.find(reinterpret_cast<uintptr_t>(type));
        if (hit != MIRKeyMap::kNotFound) return hit;
    }
    std::unique_lock<std::recursive_mutex> lk(builder_mtx_, std::defer_lock);
    if (multi) lk.lock();

    const uint64_t type_key = reinterpret_cast<uintptr_t>(type);
    const uint32_t memo_hit = memo_.find(type_key);
    if (memo_hit != MIRKeyMap::kNotFound) {
        if (tm) tm->memo.set(type_key, memo_hit);
        return memo_hit;
    }

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
            r.size = int_bits_of(k, module_.layout.is64Bit, module_.layout.windows) / 8;
            r.alignment = r.size;
            r.decl = in->is_unsigned() ? 1u : 0u;
            r.element = int_bits_of(k, module_.layout.is64Bit, module_.layout.windows);
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

    memo_.set(type_key, id);
    if (tm) tm->memo.set(type_key, id);
    return id;
}

} // namespace mir
