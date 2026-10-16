// Copyright (c) Chemical Language Foundation 2025.
//
// MIR lowering: aggregate construction, defaults and constructor proofs.
// Split out of MIRLowerer.cpp (action D4, mir-migration-evaluation.md).
// AST-aware: this is the front-end lowering boundary, never a backend.

#include "MIRLowererHelpers.h"

namespace mir {

bool MIRLowerer::apply_struct_defaults(PlaceId temp, ExtendableMembersContainerNode* def,
                                       const MIRFieldNameSet& explicit_fields,
                                       std::string& error) {
    if (!def) return true;
    // inherited bases: default-construct them (or recurse into their defaults)
    for (InheritedType& inh : def->inherited) {
        BaseType* bt = inh.type;
        ASTNode* base = bt ? bt->get_direct_linked_canonical_node() : nullptr;
        if (!base) continue;
        auto* bec = base->as_extendable_member_container();
        if (!bec) continue;
        const chem::string_view bn = bec->name_view();
        if (bn.empty()) continue;
        if (explicit_fields.contains(bn)) continue;
        const ConstantId bfc = module_.constants.add_string(
            MIR_INVALID_ID, bn.data(), static_cast<uint32_t>(bn.size()));
        const TypeId bmt = types_.map(bt);
        const PlaceId btmp = builder_->alloca(bmt, MIRStorageClass::Temporary);
        if (btmp == MIR_INVALID_ID) continue;
        FunctionDeclaration* dc = nullptr;
        if (base->kind() == ASTNodeKind::StructDecl) {
            dc = base->as_struct_def_unsafe()->default_constructor_func();
        }
        if (dc && dc->is_generated_fn()) dc = nullptr;
        // the whole-temp zero is dead when the construction writes every member:
        // a `return T{...}` constructor's result is a whole-value copy of a fully
        // defined temporary, and a generated constructor / recursive default
        // pass is covered by the predicate (action C1)
        const bool base_covers =
            dc ? constructor_writes_whole_value(dc) : default_init_writes_all_fields(bec);
        if (!base_covers) builder_->zero_init(btmp);
        if (dc) {
            builder_->init(btmp, intern_function(dc), nullptr, 0);
        } else if (!apply_struct_defaults(btmp, bec, {}, error)) {
            return false;
        }
        builder_->field_store_place(temp, bfc, btmp);
    }
    // direct members with a default value (or a member struct with a default ctor)
    for (BaseDefMember* var : def->variables()) {
        if (!var) continue;
        if (explicit_fields.contains(var->name)) continue;
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, var->name.data(), static_cast<uint32_t>(var->name.size()));
        BaseType* mt = var->known_type();
        Value* dv = var->default_value();
        if (!dv) {
            ASTNode* mn = mt ? mt->get_direct_linked_canonical_node() : nullptr;
            if (mn && mn->kind() == ASTNodeKind::StructDecl) {
                auto* msd = mn->as_struct_def_unsafe();
                FunctionDeclaration* dc = msd->default_constructor_func();
                const TypeId mtid = types_.map(mt);
                const PlaceId mtmp = builder_->alloca(mtid, MIRStorageClass::Temporary);
                // as for the inherited-base temporary: a `return T{...}`
                // constructor or a fully covering recursive default pass makes
                // the whole-temp zero dead (action C1)
                const bool member_covers =
                    (dc && !dc->is_generated_fn()) ? constructor_writes_whole_value(dc)
                                                   : default_init_writes_all_fields(msd);
                if (!member_covers) builder_->zero_init(mtmp);
                if (dc && !dc->is_generated_fn()) {
                    builder_->init(mtmp, intern_function(dc), nullptr, 0);
                } else {
                    // no explicit constructor: apply the member struct's own
                    // field defaults recursively
                    if (!apply_struct_defaults(mtmp, msd, {}, error)) return false;
                }
                builder_->field_store_place(temp, fc, mtmp);
            }
            continue;
        }
        MIRExprResult r = lower_arg_converted(dv, mt, error);
        if (!r.ok()) return false;
        if (r.kind == MIRExprKind::Place) {
            builder_->field_store_place(temp, fc, static_cast<PlaceId>(r.id));
        } else if (r.kind == MIRExprKind::Value) {
            builder_->field_store(temp, fc, static_cast<ValueId>(r.id));
        } else if (r.kind == MIRExprKind::Address) {
            builder_->field_store(temp, fc,
                                  builder_->load_indirect(static_cast<ValueId>(r.id),
                                                          types_.map(mt)));
        } else {
            error = "unsupported struct field default value";
            return false;
        }
    }
    return true;
}

// Whether every runtime member of `def` is defined by a struct literal that
// explicitly sets `explicit_fields`: each explicit field is written, each
// inherited base is written by `apply_struct_defaults` (via a default
// constructor or a recursive default pass, then copied whole into the base
// field), and each non-explicit direct member is either given a default value
// or default-constructed (a struct type). Any other member -- no default and
// not a struct, so `apply_struct_defaults` would `continue` without writing it
// -- leaves a byte that only the whole-temp `zero_init` defines. Mirrors the
// `continue` paths of `apply_struct_defaults` exactly.
bool MIRLowerer::struct_literal_defines_all_fields(ExtendableMembersContainerNode* def,
                                                   const MIRFieldNameSet& explicit_fields) {
    if (!def) return false;
    for (InheritedType& inh : def->inherited) {
        BaseType* bt = inh.type;
        ASTNode* base = bt ? bt->get_direct_linked_canonical_node() : nullptr;
        if (!base) return false;
        auto* bec = base->as_extendable_member_container();
        if (!bec) return false;
        // an empty-named base is skipped by apply_struct_defaults and stays unset
        if (bec->name_view().empty()) return false;
        // explicit => the literal wrote it; otherwise apply_struct_defaults writes
        // the base field through the base temporary it constructs.
    }
    for (BaseDefMember* var : def->variables()) {
        if (!var) continue;
        if (explicit_fields.contains(var->name)) continue;
        if (var->default_value()) continue;
        BaseType* mt = var->known_type();
        ASTNode* mn = mt ? mt->get_direct_linked_canonical_node() : nullptr;
        if (mn && mn->kind() == ASTNodeKind::StructDecl) continue;
        return false;
    }
    return true;
}

// Whether default-initializing `def` writes every runtime member of the value,
// so that a temporary which is only ever default-initialized does not need the
// whole-value `zero_init` that defines the bytes nothing writes. This mirrors
// the write set of `apply_struct_defaults` (and of the generated parameterless
// constructor, whose body is `apply_struct_defaults_ptr`) and is deliberately
// conservative:
//  * a `struct` built through a user constructor may leave a field unset, so it
//    is not provable;
//  * a member with no default value that is not a `struct` type is skipped by
//    `apply_struct_defaults` entirely (only the zero defines it);
//  * only `struct` containers qualify -- a union/variant/interface member is
//    not laid out or written the same way.
bool MIRLowerer::default_init_writes_all_fields(ExtendableMembersContainerNode* def, int depth) {
    if (!def || def->kind() != ASTNodeKind::StructDecl) return false;
    if (depth > 64) return false; // illegal by-value cycle; be conservative
    // a user (non-generated) parameterless constructor decides its own writes
    FunctionDeclaration* dc = def->default_constructor_func();
    if (dc && !dc->is_generated_fn()) return false;
    for (InheritedType& inh : def->inherited) {
        BaseType* bt = inh.type;
        ASTNode* base = bt ? bt->get_direct_linked_canonical_node() : nullptr;
        if (!base) return false;
        auto* bec = base->as_extendable_member_container();
        if (!bec) return false;
        // an empty-named base is skipped by apply_struct_defaults and stays unset
        if (bec->name_view().empty()) return false;
        if (!default_init_writes_all_fields(bec, depth + 1)) return false;
    }
    for (BaseDefMember* var : def->variables()) {
        if (!var) continue;
        if (var->default_value()) continue;
        BaseType* mt = var->known_type();
        ASTNode* mn = mt ? mt->get_direct_linked_canonical_node() : nullptr;
        if (!mn || mn->kind() != ASTNodeKind::StructDecl) return false;
        if (!default_init_writes_all_fields(mn->as_struct_def_unsafe(), depth + 1)) return false;
    }
    return true;
}

// Whether a constructor's result is produced by a whole-value copy of a fully
// lowered temporary, which defines every byte of the result -- so a `zero_init`
// guarding the result temporary at the call site is dead.
//
// Every constructor in this codebase (and the documented style) has the body
// `{ return T { field: expr, ... } }`. Such a literal is lowered to its own
// temporary, and `return` in a struct-returning function lowers to a
// *whole-value store* of that temporary into the sret destination
// (`MIRLowerer::lower_stmt`, `ReturnStmt` with `sret_` set), so the store
// overwrites every byte a caller-side zero would have written. A constructor
// that assigns `self.field` instead, that can also exit by another path, or
// whose return value is not a literal of the same container (e.g. one needing an
// implicit conversion) is not provable and keeps the zero.
//
// If the return lowering ever becomes an in-place construction straight into
// the sret (no temporary and no store), this proof must be revisited.
bool MIRLowerer::function_body_is_readable(FunctionDeclaration* fn) {
    if (!fn) return false;
    if (fn->is_body_retained()) return true;
    if (!current_module_file_scopes_) return false;
    ASTNode* file_scope = fn->get_ancestor_by_kind(ASTNodeKind::FileScope);
    return file_scope &&
           current_module_file_scopes_->find(file_scope) != current_module_file_scopes_->end();
}

void MIRLowerer::prime_constructor_fact(FunctionDeclaration* fn) {
    if (!fn || !fn->is_constructor_fn() || fn->is_ctor_whole_value_known()) return;
    if (!fn->body.has_value()) return;
    if (!function_body_is_readable(fn)) return;
    fn->set_ctor_writes_whole_value(constructor_writes_whole_value(fn));
}

bool MIRLowerer::construction_writes_whole_value(FunctionDeclaration* ctor,
                                                 ExtendableMembersContainerNode* cdef) {
    bool ctor_covers = false;
    if (ctor->is_ctor_whole_value_known()) {
        // the proof ran earlier, while the constructor's own module still had its
        // body; a later module must not re-read a possibly-freed body
        ctor_covers = ctor->ctor_writes_whole_value();
    } else if (function_body_is_readable(ctor)) {
        ctor_covers = constructor_writes_whole_value(ctor);
        ctor->set_ctor_writes_whole_value(ctor_covers);
    }
    return ctor_covers ||
           (ctor->is_generated_fn() && default_init_writes_all_fields(cdef));
}

bool MIRLowerer::constructor_writes_whole_value(FunctionDeclaration* ctor) {
    if (!ctor || !ctor->body.has_value()) return false;
    // The body of a non-retained function is allocated on the module allocator,
    // which `process_module_tcc` clears once that module is compiled. A
    // reference from a later module keeps only the job-allocated declaration
    // alive, so its `body`'s statement nodes are already freed — reading them is
    // a use-after-free. Only inspect a body that is retained for the whole job
    // or was parsed as part of the module currently being lowered.
    if (!function_body_is_readable(ctor)) return false;
    auto* def = ctor->parent() ? ctor->parent()->as_extendable_member_container() : nullptr;
    if (!def) return false;
    const Scope& body = ctor->body.value();
    if (body.nodes.empty()) return false;

    // Shape 1 (the common one): the body is only `return T { ... }` statements.
    // Each lowers to a whole-value store of the literal's own (fully defined)
    // temporary into the sret destination.
    bool all_return_literals = true;
    for (ASTNode* node : body.nodes) {
        if (!node || node->kind() != ASTNodeKind::ReturnStmt) {
            all_return_literals = false;
            break;
        }
        auto* ret = static_cast<ReturnStatement*>(node);
        Value* v = ret->value;
        if (!v || v->val_kind() != ValueKind::StructValue) {
            all_return_literals = false;
            break;
        }
        if (v->as_struct_value_unsafe()->linked_extendable() != def) {
            all_return_literals = false;
            break;
        }
    }
    if (all_return_literals) return true;

    // Shape 3: `... <setup stmts>; return T { ... }`. The only statement that
    // writes the hidden sret is a `return`, so a body whose *only* return is a
    // final `return T { ... }` of the same container covers the result no matter
    // what precedes it -- the preceding statements write locals, not the sret.
    // Restricted to `var`/expression statements before the return, so a nested
    // `return` (inside an `if`/loop) cannot hide another exit path.
    if (!body.nodes.empty()) {
        bool setup_only = true;
        for (size_t i = 0; i + 1 < body.nodes.size(); ++i) {
            ASTNode* n = body.nodes[i];
            if (!n || (n->kind() != ASTNodeKind::VarInitStmt &&
                       n->kind() != ASTNodeKind::ValueWrapper &&
                       n->kind() != ASTNodeKind::AccessChainNode)) {
                setup_only = false;
                break;
            }
        }
        ASTNode* last = body.nodes.back();
        if (setup_only && last && last->kind() == ASTNodeKind::ReturnStmt) {
            auto* ret = static_cast<ReturnStatement*>(last);
            Value* v = ret->value;
            if (v && v->val_kind() == ValueKind::StructValue &&
                v->as_struct_value_unsafe()->linked_extendable() == def) {
                return true;
            }
        }
    }

    // Shape 2: `... var m = T { ... }; <no-write stmts>; return m`. `m` is fully
    // defined by its literal -- a literal that does not cover every field is
    // itself zeroed, so either way the value is complete -- and `return m` is a
    // whole-value store of `m` into the sret, so the result temporary needs no
    // caller-side zero. Any statement that could leave `m` partially defined
    // (an assignment to it, an `if`, a loop, an early return) disqualifies the
    // body; a bare `var` or expression/call statement is harmless (writing a
    // field of `m` cannot un-define the bytes already written).
    for (size_t i = 0; i < body.nodes.size(); ++i) {
        ASTNode* node = body.nodes[i];
        if (!node) return false;
        if (node->kind() == ASTNodeKind::ReturnStmt) {
            if (i + 1 != body.nodes.size()) return false; // the return must be last
            auto* ret = static_cast<ReturnStatement*>(node);
            Value* v = ret->value;
            // Resolve `return m`: a bare identifier, or a one-element access
            // chain to it. A chain whose `values` were consumed while lowering
            // the constructor is resolved to the body's unique local built from
            // a literal of the container (a constructor returns its own value).
            ASTNode* linked = nullptr;
            if (v && v->val_kind() == ValueKind::Identifier) {
                linked = v->linked_node();
            } else if (v && v->val_kind() == ValueKind::AccessChain) {
                auto* ch = v->as_access_chain_unsafe();
                if (ch->values.empty()) {
                    // the chain was consumed while lowering the constructor:
                    // resolve to the body's unique local built from a literal of
                    // the container (a constructor returns its own value).
                    for (ASTNode* n : body.nodes) {
                        if (!n || n->kind() != ASTNodeKind::VarInitStmt) continue;
                        auto* cand = n->as_var_init_unsafe();
                        if (!cand->value || cand->value->val_kind() != ValueKind::StructValue) {
                            continue;
                        }
                        if (cand->value->as_struct_value_unsafe()->linked_extendable() != def) {
                            continue;
                        }
                        if (linked) {
                            linked = nullptr; // ambiguous: reject
                            break;
                        }
                        linked = n;
                    }
                } else {
                    Value* leaf = ch->values.back();
                    if (leaf && leaf->val_kind() == ValueKind::Identifier) {
                        linked = leaf->linked_node();
                    }
                }
            }
            if (!linked || linked->kind() != ASTNodeKind::VarInitStmt) return false;
            auto* vi = linked->as_var_init_unsafe();
            if (!vi->value || vi->value->val_kind() != ValueKind::StructValue) return false;
            if (vi->value->as_struct_value_unsafe()->linked_extendable() != def) return false;
            // `m` must be a direct statement of this body, so no other statement
            // here could alias its name
            bool declared_here = false;
            for (ASTNode* n : body.nodes) {
                if (n == linked) {
                    declared_here = true;
                    break;
                }
            }
            if (!declared_here) return false;
            continue;
        }
        if (node->kind() == ASTNodeKind::VarInitStmt) continue;
        if (node->kind() == ASTNodeKind::ValueWrapper) continue;
        if (node->kind() == ASTNodeKind::AccessChainNode) continue;
        return false;
    }
    return true;
}

bool MIRLowerer::apply_struct_defaults_ptr(ValueId self_ptr, ExtendableMembersContainerNode* def,
                                           std::string& error) {
    if (!def) return true;
    for (InheritedType& inh : def->inherited) {
        BaseType* bt = inh.type;
        ASTNode* base = bt ? bt->get_direct_linked_canonical_node() : nullptr;
        if (!base) continue;
        auto* bec = base->as_extendable_member_container();
        if (!bec) continue;
        const chem::string_view bn = bec->name_view();
        if (bn.empty()) continue;
        const ConstantId bfc = module_.constants.add_string(
            MIR_INVALID_ID, bn.data(), static_cast<uint32_t>(bn.size()));
        const TypeId bmt = types_.map(bt);
        const ValueId baddr = builder_->field_addr_ptr(self_ptr, bfc, bmt);
        FunctionDeclaration* dc = nullptr;
        if (base->kind() == ASTNodeKind::StructDecl) {
            dc = base->as_struct_def_unsafe()->default_constructor_func();
        }
        if (dc && !dc->is_generated_fn()) {
            MIROperand a = MIROperand::value(baddr, types_.pointer_type(bmt, true));
            builder_->call_scalar(intern_function(dc), types_.void_type(), &a, 1);
        } else if (!apply_struct_defaults_ptr(baddr, bec, error)) {
            return false;
        }
    }
    for (BaseDefMember* var : def->variables()) {
        if (!var) continue;
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, var->name.data(), static_cast<uint32_t>(var->name.size()));
        BaseType* mt = var->known_type();
        Value* dv = var->default_value();
        if (!dv) {
            ASTNode* mn = mt ? mt->get_direct_linked_canonical_node() : nullptr;
            if (mn && mn->kind() == ASTNodeKind::StructDecl) {
                auto* msd = mn->as_struct_def_unsafe();
                FunctionDeclaration* dc = msd->default_constructor_func();
                const TypeId mtid = types_.map(mt);
                const ValueId maddr = builder_->field_addr_ptr(self_ptr, fc, mtid);
                if (dc && !dc->is_generated_fn()) {
                    MIROperand a = MIROperand::value(maddr, types_.pointer_type(mtid, true));
                    builder_->call_scalar(intern_function(dc), types_.void_type(), &a, 1);
                } else if (!apply_struct_defaults_ptr(maddr, msd, error)) {
                    return false;
                }
            }
            continue;
        }
        MIRExprResult r = lower_arg_converted(dv, mt, error);
        if (!r.ok()) return false;
        if (r.kind == MIRExprKind::Place) {
            builder_->field_store_ptr(self_ptr, fc,
                                      builder_->load(static_cast<PlaceId>(r.id), types_.map(mt)));
        } else if (r.kind == MIRExprKind::Value) {
            builder_->field_store_ptr(self_ptr, fc, static_cast<ValueId>(r.id));
        } else if (r.kind == MIRExprKind::Address) {
            builder_->field_store_ptr(
                self_ptr, fc,
                builder_->load_indirect(static_cast<ValueId>(r.id), types_.map(mt)));
        } else {
            error = "unsupported struct field default value";
            return false;
        }
    }
    return true;
}

bool MIRLowerer::store_anon_aggregate(ValueId addr, StructValue* sv, std::string& error,
                                      bool enclosing_zeroed) {
    if (!sv) return true;
    for (auto& kv : sv->values) {
        const StructMemberInitializer& init = kv.second;
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, init.name.data(), static_cast<uint32_t>(init.name.size()));
        BaseType* mt = nullptr;
        {
            auto mtw = sv->child_type_w_index(init.name);
            if (mtw.second != -1) mt = mtw.first;
        }
        // An empty array literal for a fixed-size array field inside an
        // anonymous aggregate (`buffer : []` in `std::string`'s SSO storage)
        // means "all zeroes". When the enclosing temporary is already zeroed
        // the field is defined and the store is skipped; otherwise zero the
        // field through its address, instead of the zeroed scratch array plus
        // whole-width copy `lower_arg_converted` would emit (action C1).
        if (init.value && init.value->val_kind() == ValueKind::ArrayValue &&
            init.value->as_array_value_unsafe()->values.empty()) {
            auto* fat = mt ? mt->canonical()->as_array_type() : nullptr;
            if (fat && fat->has_array_size()) {
                if (enclosing_zeroed) continue;
                const TypeId ft = types_.map(mt);
                if (ft != MIR_INVALID_ID && ft < module_.types.size()) {
                    const ValueId faddr = builder_->field_addr_ptr(addr, fc, ft);
                    const TypeId u8 = types_.char_type();
                    const TypeId usz = types_.u64_type();
                    const ValueId byte = builder_->const_int(
                        u8, module_.constants.add_int(u8, 0));
                    const ValueId bytes = builder_->const_int(
                        usz, module_.constants.add_int(
                                 usz, module_.types.get(ft).size));
                    builder_->memset_ptr(faddr, byte, bytes);
                    continue;
                }
            }
        }
        if (init.value && init.value->val_kind() == ValueKind::StructValue) {
            ASTNode* mn = mt ? mt->get_direct_linked_canonical_node() : nullptr;
            if (mn && (mn->kind() == ASTNodeKind::UnnamedUnion ||
                       mn->kind() == ASTNodeKind::UnnamedStruct)) {
                const TypeId ft = types_.map(mt);
                const ValueId faddr = builder_->field_addr_ptr(addr, fc, ft);
                if (!store_anon_aggregate(faddr, init.value->as_struct_value_unsafe(), error,
                                          enclosing_zeroed)) {
                    return false;
                }
                continue;
            }
        }
        MIRExprResult r = mt ? lower_arg_converted(init.value, mt, error)
                             : lower_expr(init.value, error);
        if (!r.ok()) return false;
        const TypeId ft = mt ? types_.map(mt) : r.type;
        if (r.kind == MIRExprKind::Value) {
            builder_->field_store_ptr(addr, fc, static_cast<ValueId>(r.id));
        } else if (r.kind == MIRExprKind::Place) {
            builder_->field_store_ptr(addr, fc,
                                      builder_->load(static_cast<PlaceId>(r.id), ft));
        } else if (r.kind == MIRExprKind::Address) {
            builder_->field_store_ptr(addr, fc,
                                      builder_->load_indirect(static_cast<ValueId>(r.id), ft));
        } else {
            error = "unsupported anonymous aggregate field initializer";
            return false;
        }
    }
    return true;
}


} // namespace mir
