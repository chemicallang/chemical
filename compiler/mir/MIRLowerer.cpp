// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"
#include "MIRLowererInternal.h"
#include "MIRLowererHelpers.h"
#include "MIRABI.h"
#include "MIRPrototype.h"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <chrono>

namespace mir {

// The lowerer currently lowering on this thread. The translation driver wires
// the shared comptime callbacks once (they capture the *driver* lowerer); a
// parallel worker lowers through its own lowerer, so the callbacks consult this
// to publish/read the worker's folded comptime parameter bindings rather than
// the driver's. Null outside a lowering, in which case the callbacks fall back
// to the driver (the serial behaviour). Kept in a translation unit (not a
// header) so every backend shares one definition.
static thread_local MIRLowerer* tls_current_lowering = nullptr;

MIRLowerer* MIRLowerer::current_lowering() noexcept {
    return tls_current_lowering;
}

MIRLowerer* MIRLowerer::set_current_lowering(MIRLowerer* l) noexcept {
    MIRLowerer* prev = tls_current_lowering;
    tls_current_lowering = l;
    return prev;
}

} // namespace mir

#include "ast/base/BaseType.h"
#include "ast/types/GenericType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/BitwiseNot.h"
#include "ast/values/Expression.h"
#include "ast/values/CastedValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ReferenceOfValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/DynamicValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/StructMemberInitializer.h"
#include "ast/values/ValueNode.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/Scope.h"
#include "ast/structures/BlockScope.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/If.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/EnumMember.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/InlineAsmStmt.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/values/IncDecValue.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/values/OffsetOfValue.h"
#include "ast/values/UnsafeValue.h"
#include "ast/values/ZeroedValue.h"
#include "ast/values/IfValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/VariantCase.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/ExtractionValue.h"
#include "ast/values/NewTypedValue.h"
#include "ast/values/NewValue.h"
#include "ast/values/PlacementNewValue.h"
#include "ast/statements/DestructStmt.h"
#include "ast/statements/DeallocStmt.h"
#include "ast/statements/PlacementNewNode.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/values/EmbeddedValue.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/values/SwitchValue.h"
#include "ast/values/LoopValue.h"
#include "ast/values/PatternMatchExpr.h"
#include "ast/values/InValue.h"
#include "ast/values/BlockValue.h"
#include "ast/statements/PatternMatchExprNode.h"
#include "ast/statements/Return.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/values/AwaitExpression.h"
#include "compiler/async/AsyncCTypes.h"
#include "compiler/async/AwaitNormalizePass.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/ForInLoop.h"
#include "ast/structures/InheritedType.h"
#include "ast/structures/BaseDefMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/types/StructType.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/symres/CoreNodes.h"
#include "MIREmitter.h"

namespace mir {

// --- optional D5 instrumentation (see MIRHotOps.h) --------------------------
#ifdef MIR_OPS_COUNTERS
MIRHotOps g_hot_ops;
namespace {
struct MIRHotOpsPrinter {
    ~MIRHotOpsPrinter() {
        if (std::getenv("MIR_OPS") == nullptr) return;
        std::cerr << "[MIR_OPS] functions=" << g_hot_ops.functions
                  << " instrs=" << g_hot_ops.instrs
                  << " lower_ms=" << (g_hot_ops.lower_ns / 1000000ULL)
                  << " place_lookup=" << g_hot_ops.place_lookup
                  << " place_bind=" << g_hot_ops.place_bind
                  << " name_lookup=" << g_hot_ops.name_lookup
                  << " name_bind=" << g_hot_ops.name_bind
                  << " type_map=" << g_hot_ops.type_map
                  << " intern=" << g_hot_ops.intern << std::endl;
    }
} g_hot_ops_printer;
} // namespace
#endif
// ---------------------------------------------------------------------------


PlaceId MIRLowerer::place_for_linked(ASTNode* linked) const {
    if (!linked) return MIR_INVALID_ID;
    MIR_OP(place_lookup);
    auto it = var_places_.find(linked);
    if (it == var_places_.end()) return MIR_INVALID_ID;
    return it->second;
}

PlaceId MIRLowerer::resolve_place_silent(Value* v) {
    if (!v) return MIR_INVALID_ID;
    Value* t = v;
    if (t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (t->val_kind() != ValueKind::Identifier) return MIR_INVALID_ID;
    return place_for_linked(t->as_identifier_unsafe()->linked);
}

PlaceId MIRLowerer::resolve_place(Value* v, std::string& error) {
    if (!v) {
        error = "null place expression";
        return MIR_INVALID_ID;
    }
    Value* t = v;
    if (t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (t->val_kind() != ValueKind::Identifier) {
        error = "expression is not a simple place (kind " +
                std::to_string(static_cast<int>(t->val_kind())) + ")";
        return MIR_INVALID_ID;
    }
    auto* id = t->as_identifier_unsafe();
    const PlaceId p = place_for_linked(id->linked);
    if (p == MIR_INVALID_ID) {
        error = "identifier '" + std::string(id->value.data(), id->value.size()) +
                "' does not resolve to a local place";
        return MIR_INVALID_ID;
    }
    return p;
}

PlaceId MIRLowerer::resolve_place_or_lower(Value* v, std::string& error) {
    const PlaceId direct = resolve_place(v, error);
    if (direct != MIR_INVALID_ID) return direct;
    // member access on the result of an expression (e.g. a function call that
    // returns an aggregate temporary): lower it and use its place if any.
    Value* t = v;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t) return MIR_INVALID_ID;
    std::string inner;
    MIRExprResult r = lower_expr(t, inner);
    if (r.ok() && r.kind == MIRExprKind::Place) return static_cast<PlaceId>(r.id);
    error = "expression is not a simple place (kind " +
            std::to_string(static_cast<int>(t->val_kind())) + ")" +
            (inner.empty() ? std::string() : ("; " + inner));
    return MIR_INVALID_ID;
}

FunctionDeclaration* MIRLowerer::resolve_method_fallback(BaseType* recv_type,
                                                         const std::string& name) {
    if (name.empty()) return nullptr;
    if (recv_type) {
        ASTNode* node = recv_type->get_direct_linked_canonical_node();
        if (!node) node = recv_type->get_direct_linked_node();
        if (node) {
            MembersContainer* mc = node->get_members_container();
            if (mc) {
                FunctionDeclaration* f =
                    mc->any_child_function(chem::string_view(name.data(), name.size()));
                if (f) return f;
            }
        }
    }
    // last resort: a top-level function with this name (extension functions on
    // non-static interfaces are not registered on the receiver's container)
    if (symbol_lookup_) return symbol_lookup_(name);
    return nullptr;
}

ExtendableMembersContainerNode* MIRLowerer::receiver_user_container(BaseType* recv_type) {
    if (!recv_type) return nullptr;
    BaseType* t = recv_type->canonical();
    if (!t) return nullptr;
    if (t->kind() == BaseTypeKind::Pointer) {
        t = t->as_pointer_type()->type ? t->as_pointer_type()->type->canonical() : nullptr;
    } else if (t->kind() == BaseTypeKind::Reference) {
        t = t->as_reference_type()->type ? t->as_reference_type()->type->canonical() : nullptr;
    }
    if (!t) return nullptr;
    ASTNode* n = t->get_direct_linked_canonical_node();
    if (!n) return nullptr;
    if (n->kind() == ASTNodeKind::InterfaceDecl) {
        // inside a default method the receiver is the interface `self`; the
        // emitted method belongs to the active impl struct
        auto* iface = n->as_interface_def_unsafe();
        if (iface->active_user && !iface->is_static()) return iface->active_user;
    }
    return n->as_extendable_member_container();
}

SymbolId MIRLowerer::intern_method_function(FunctionDeclaration* decl, BaseType* recv_type) {
    ASTNode* parent = decl ? decl->parent() : nullptr;
    if (!parent || parent->kind() != ASTNodeKind::InterfaceDecl) {
        return intern_function(decl);
    }
    auto* iface = parent->as_interface_def_unsafe();
    if (iface->is_static()) return intern_function(decl);
    ExtendableMembersContainerNode* user = receiver_user_container(recv_type);
    if (!user) return intern_function(decl);

    // an interface default method is emitted once per implementing struct; the
    // mangler keys its name off `active_user`, so set it while mangling
    // `active_user` and the mangler are shared translator state: hold the
    // visitor lock across setting it and mangling (the mangler callback re-locks
    // recursively).
    std::unique_lock<std::recursive_mutex> visitor_lock;
    if (visitor_mutex_) visitor_lock = std::unique_lock<std::recursive_mutex>(*visitor_mutex_);
    const auto prev = iface->active_user;
    iface->active_user = user;
    const std::string& name = mangle(decl);
    iface->active_user = prev;
    if (name.empty()) return intern_function(decl);

    auto it = named_method_symbols_.find(name);
    if (it != named_method_symbols_.end()) return it->second;

    TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    MIRScratch<TypeId> _ptypes(*this);
    std::vector<TypeId>& ptypes = _ptypes.v;
    for (FunctionParam* p : decl->params) {
        ptypes.push_back(p && p->type ? types_.map(p->type) : types_.opaque_type());
    }
    TypeId ftype = types_.function_signature(ret, ptypes, decl->isVariadic());
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = (decl->is_extern() || decl->is_linkage_public()) ? MIRLinkage::External
                                                                  : MIRLinkage::Internal;
    rec.has_body = !decl->is_extern();
    rec.type = ftype;
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    named_method_symbols_[name] = s;
    const uint32_t sig = intern_region_signature(decl);
    if (sig != MIR_INVALID_ID) module_.set_symbol_region_signature(s, sig);
    return s;
}

MIRExprResult MIRLowerer::lower_address_of(Value* inner, std::string& error,
                                          bool raw_mutable) {
    Value* t = inner;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() >= 2) {
            // `&raw obj.field` / `&raw ptr->field`: return the last field address
            MIRExprResult first = lower_expr(c->values[0], error);
            if (!first.ok()) return first;
            PlaceId cur_place = MIR_INVALID_ID;
            ValueId cur_ptr = MIR_NULL;
            bool have_ptr = false;
            if (first.kind == MIRExprKind::Place) {
                cur_place = static_cast<PlaceId>(first.id);
                if (raw_mutable) mark_raw_escaped(cur_place);
            } else if (first.kind == MIRExprKind::Value ||
                       first.kind == MIRExprKind::Address) {
                cur_ptr = static_cast<ValueId>(first.id);
                have_ptr = true;
            } else {
                error = "address-of member base is not a place or pointer";
                return MIRExprResult::error();
            }
            for (size_t i = 1; i < c->values.size(); ++i) {
                std::string fname;
                if (!member_field_path(c->values[i - 1], c->values[i], fname, error)) return MIRExprResult::error();
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                const TypeId ft = types_.map(c->values[i]->getType());
                const ValueId faddr = have_ptr ? builder_->field_addr_ptr(cur_ptr, fc, ft)
                                               : builder_->field_addr(cur_place, fc, ft);
                const MIRTypeRecord& ftr = module_.types.get(ft);
                const bool last = (i + 1 == c->values.size());
                if (last) {
                    if (ftr.kind == MIRTypeKind::Reference) {
                        // `&raw ref` on a reference field yields the referent
                        return MIRExprResult::value(builder_->load_indirect(faddr, ft), ft);
                    }
                    // the address of the last field (`&raw obj.field`)
                    return MIRExprResult::value(faddr, types_.pointer_type(ft, true));
                }
                // an intermediate pointer/reference field is dereferenced so the
                // next field is reached through it (`a.b->c`)
                cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                           ftr.kind == MIRTypeKind::Reference)
                              ? builder_->load_indirect(faddr, ft)
                              : faddr;
                have_ptr = true;
            }
        }
    }
    if (t && t->val_kind() == ValueKind::DereferenceValue) {
        // `&*p` / `&raw *p` -> the pointer p itself
        auto* d = t->as_dereference_value_unsafe();
        MIRExprResult p = lower_expr(d->getValue(), error);
        if (!p.ok()) return p;
        return MIRExprResult::value(static_cast<ValueId>(p.id), p.type);
    }
    if (t && t->val_kind() == ValueKind::IndexOperator) {
        // `&raw arr[i]` / `&mut ptr[i]` -> the element address, not the element value
        auto* io = t->as_index_op_unsafe();
        MIRExprResult base = lower_expr(io->parent_val, error);
        if (!base.ok()) return base;
        maybe_register_call_temp(io->parent_val, base);
        if (base.kind == MIRExprKind::Place) {
            const TypeId bt = builder_->function().places[base.id].type;
            if (abi::is_struct_like(module_, bt)) {
                // `&x[i]` on a struct uses the `Index`/`IndexMut` method, which
                // already returns a pointer to the element
                BaseType* recv_t = io->parent_val ? io->parent_val->getType() : nullptr;
                FunctionDeclaration* fd = resolve_method_fallback(recv_t, "index_mut");
                if (!fd) fd = resolve_method_fallback(recv_t, "index");
                if (fd) {
                    VariableIdentifier op_id(chem::string_view("index"), nullptr, ZERO_LOC);
                    op_id.linked = fd;
                    FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
                    op_call.values.push_back(io->idx);
                    return lower_method_call(base, recv_t, &op_call, error);
                }
            }
        }
        MIRExprResult idx = lower_expr(io->idx, error);
        if (!idx.ok()) return idx;
        const TypeId et = types_.map(io->getType());
        const TypeId ptrt = types_.pointer_type(et, true);
        if (base.kind == MIRExprKind::Place) {
            if (raw_mutable) mark_raw_escaped(static_cast<PlaceId>(base.id));
            return MIRExprResult::value(
                builder_->index_addr(static_cast<PlaceId>(base.id),
                                     static_cast<ValueId>(idx.id), et),
                ptrt);
        }
        if (base.kind == MIRExprKind::Value || base.kind == MIRExprKind::Address) {
            return MIRExprResult::value(
                builder_->index_addr_ptr(static_cast<ValueId>(base.id),
                                         static_cast<ValueId>(idx.id), et),
                ptrt);
        }
        error = "address-of index base is not a place or pointer";
        return MIRExprResult::error();
    }
    if (t && t->val_kind() == ValueKind::Identifier) {
        auto* id = t->as_identifier_unsafe();
        if (id->linked && id->linked->kind() == ASTNodeKind::VariantCaseVariable) {
            return lower_variant_payload(id->linked->as_variant_case_var_unsafe(), true, error);
        }
        if (id->linked && id->linked->kind() == ASTNodeKind::PatternMatchId) {
            auto* pmid = id->linked->as_patt_match_id_unsafe();
            return lower_pattern_payload(pmid->matchExpr, pmid->member_param, true, error);
        }
        if (id->linked && id->linked->kind() == ASTNodeKind::CapturedVariable) {
            return lower_captured_variable(id->linked->as_captured_var_unsafe(), true, error);
        }
        if (id->linked && id->linked->kind() == ASTNodeKind::ForInLoopStmt) {
            const PlaceId place = place_for_linked(id->linked);
            if (place != MIR_INVALID_ID) {
                const TypeId pt = builder_->function().places[place].type;
                return MIRExprResult::value(builder_->load(place, pt), pt);
            }
        }
        if (is_implicit_field_node(id->linked) && current_self_param_) {
            // `&field` inside a method is `&self.field`
            VariableIdentifier self_id(current_self_param_->name,
                                       current_self_param_->type, ZERO_LOC);
            self_id.linked = current_self_param_;
            std::string fname;
            if (!member_field_path(&self_id, id, fname, error)) {
                return MIRExprResult::error();
            }
            const ConstantId fc = module_.constants.add_string(
                MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
            const TypeId ft = types_.map(id->getType());
            MIRExprResult sb = lower_expr(&self_id, error);
            if (!sb.ok()) return sb;
            const ValueId faddr = (sb.kind == MIRExprKind::Place)
                                      ? builder_->field_addr(static_cast<PlaceId>(sb.id), fc, ft)
                                      : builder_->field_addr_ptr(static_cast<ValueId>(sb.id), fc, ft);
            const MIRTypeRecord& ftr = module_.types.get(ft);
            if (ftr.kind == MIRTypeKind::Reference) {
                // `&raw ref` on a reference field yields the referent address
                // (the stored reference value), not the field's own address
                return MIRExprResult::value(builder_->load_indirect(faddr, ft), ft);
            }
            return MIRExprResult::value(faddr, types_.pointer_type(ft, true));
        }
        const PlaceId p = place_for_linked(id->linked);
        if (p != MIR_INVALID_ID) {
            if (raw_mutable) mark_raw_escaped(p);
            const TypeId pt = builder_->function().places[p].type;
            const MIRTypeRecord& pr = module_.types.get(pt);
            const TypeId ast_t = types_.map(id->getType());
            if (pr.kind == MIRTypeKind::Reference ||
                (abi::is_aggregate(module_, ast_t) && pr.kind == MIRTypeKind::Pointer)) {
                // a reference denotes its referent, so `&x` is the reference value;
                // an aggregate parameter passed by pointer likewise
                return MIRExprResult::value(builder_->load(p, pt), pt);
            }
            const TypeId ptrt = types_.pointer_type(pt, true);
            return MIRExprResult::value(builder_->address_of(p, ptrt), ptrt);
        }
        const PlaceId pn = place_for_name(id->value);
        if (pn != MIR_INVALID_ID) {
            const TypeId pt = builder_->function().places[pn].type;
            const MIRTypeRecord& pr = module_.types.get(pt);
            const TypeId ast_t = types_.map(id->getType());
            if (pr.kind == MIRTypeKind::Reference ||
                (abi::is_aggregate(module_, ast_t) && pr.kind == MIRTypeKind::Pointer)) {
                return MIRExprResult::value(builder_->load(pn, pt), pt);
            }
            const TypeId ptrt = types_.pointer_type(pt, true);
            return MIRExprResult::value(builder_->address_of(pn, ptrt), ptrt);
        }
        if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
            auto* vi = id->linked->as_var_init();
            const SymbolId gs = intern_global(vi);
            const TypeId vt = types_.map(id->getType());
            const TypeId ptrt = types_.pointer_type(vt, false);
            return MIRExprResult::address(builder_->global_addr(gs, ptrt), ptrt);
        }
        const std::string gname(id->value.data(), id->value.size());
        if (!gname.empty()) {
            const SymbolId gs = intern_named_global(gname);
            const TypeId vt = types_.map(id->getType());
            const TypeId ptrt = types_.pointer_type(vt, false);
            return MIRExprResult::address(builder_->global_addr(gs, ptrt), ptrt);
        }
    }
    MIRExprResult in = lower_expr(inner, error);
    if (!in.ok()) return in;
    // `&create()` keeps the call's temporary alive until the statement ends
    maybe_register_call_temp(inner, in);
    return in;
}

void MIRLowerer::mark_moved(PlaceId place) {
    for (auto& d : destructibles_) {
        if (d.place == place) {
            builder_->set_drop(d.flag, false);
            set_drop_fields(d.fields, false);
            return;
        }
    }
}

void MIRLowerer::clear_moved_ref(Value* value) {
    Value* t = value;
    for (int guard = 0; guard < 4 && t; ++guard) {
        if (t->val_kind() == ValueKind::AccessChain) {
            auto* c = t->as_access_chain_unsafe();
            if (c->values.size() == 1) { t = c->values[0]; continue; }
            break;
        }
        if (t->val_kind() == ValueKind::UnsafeValue) {
            auto* u = static_cast<UnsafeValue*>(t);
            if (u->getValue()) { t = u->getValue(); continue; }
            break;
        }
        break;
    }
    if (!t || t->val_kind() != ValueKind::Identifier) return;
    auto* id = t->as_identifier_unsafe();
    if (!id->linked || id->linked->kind() != ASTNodeKind::PatternMatchId) return;
    PatternMatchExpr* pm = id->linked->as_patt_match_id_unsafe()->matchExpr;
    if (!pm || !pm->expression) return;
    Value* src = pm->expression;
    for (int guard = 0; guard < 4 && src; ++guard) {
        if (src->val_kind() == ValueKind::AccessChain) {
            auto* c = src->as_access_chain_unsafe();
            if (c->values.size() == 1) { src = c->values[0]; continue; }
            break;
        }
        break;
    }
    if (!src || src->val_kind() != ValueKind::Identifier) return;
    auto* sid = src->as_identifier_unsafe();
    PlaceId sp = place_for_linked(sid->linked);
    if (sp == MIR_INVALID_ID) sp = place_for_name(sid->value);
    if (sp != MIR_INVALID_ID) mark_moved(sp);
}

void MIRLowerer::emit_drops() {
    // local/parameter scope destruction is reverse order of initialization
    // (mir-design.md §7.4); emit_drops_from already iterates innermost-first
    emit_drops_from(0);
}

void MIRLowerer::emit_storage_dead_from(size_t mark) {
    for (size_t i = scope_places_.size(); i-- > mark;) {
        if (builder_->current_block_terminated()) break;
        builder_->storage_dead(scope_places_[i]);
    }
}

void MIRLowerer::emit_drops_from(size_t mark) {
    for (size_t i = destructibles_.size(); i-- > mark;) {
        MIRDestructible& d = destructibles_[i];
        if (!d.fields.empty() && !d.raw_escaped) {
            // a field-wise destructible destroys only the initialized fields
            destroy_field_wise(d);
            continue;
        }
        const ValueId fv = builder_->load(d.flag, types_.bool_type());
        builder_->drop(d.place, d.dtor, fv);
    }
}




// A method of an `impl StaticInterface for T` uses the interface's erased self
// ABI (`void**`); see MIRDeclEmitter's identical helper.
static FunctionDeclaration* lower_static_iface_base_method(FunctionDeclaration* decl) {
    if (!decl) return nullptr;
    ASTNode* parent = decl->parent();
    if (!parent || parent->kind() != ASTNodeKind::ImplDecl) return nullptr;
    auto* impl = parent->as_impl_def_unsafe();
    if (!impl->interface_type) return nullptr;
    auto* iface = impl->interface_type->get_direct_linked_interface();
    if (!iface || !iface->is_static()) return nullptr;
    return iface->direct_child_function(decl->name_view());
}

namespace {

/** The declaration spells a lifetime on its return type (`&'a T` / `Foo<'a>`). */
bool declaration_has_return_lifetime(BaseType* type) {
    if (type == nullptr) return false;
    if (type->kind() == BaseTypeKind::Reference) {
        return !type->as_reference_type()->lifetime.empty();
    }
    if (type->kind() == BaseTypeKind::Generic) {
        for (chem::string_view lt : type->as_generic_type()->lifetime_args) {
            if (!lt.empty()) return true;
        }
    }
    return false;
}

} // namespace

bool MIRLowerer::lower_static_interface_stub(FunctionDeclaration* decl, MIRArena& arena,
                                             MIRFunction& func, std::string& error) {
    if (!decl) {
        error = "null static interface method";
        return false;
    }
    // A default method with a body is lowered as-is: it *is* the definition a
    // caller reaches when nothing implements the interface.
    if (decl->body.has_value()) {
        if (!lower_function(decl, arena, func, error)) return false;
        func.flags |= MF_WEAK_STUB;
        return true;
    }

    // Bodyless method: no implementation exists anywhere, so give it a stub.
    // The signature is `intern_function`'s — that is what every call site of a
    // not-yet-defined function already assumes — and the body is trivial.
    arena_ = &arena;
    current_func_ = decl;
    current_fn_is_comptime_ = decl->is_comptime();
    if (set_current_func_) set_current_func_(decl);
    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    scope_places_.clear();
    stmt_temps_.clear();

    func.symbol = intern_function(decl);
    // Region signatures are built from the declaration before any body
    // lowering, so MIRBorrowCheck (M4) never has to read the AST.
    func.region_signature = module_.symbol_region_signature(func.symbol);
    // A generic declaration is interned before its return type is linked, so a
    // cached signature can lack the return region. Rebuild from the linked
    // declaration when that happened (S4.6a aggregate returns).
    if (func.region_signature != MIR_INVALID_ID &&
        func.region_signature < module_.region_signatures.size() &&
        module_.region_signatures[func.region_signature].return_region ==
            MIR_INVALID_ID &&
        decl->returnType != nullptr) {
        if (declaration_has_return_lifetime(decl->returnType)) {
            const uint32_t fresh = intern_region_signature(decl);
            if (fresh != MIR_INVALID_ID &&
                fresh < module_.region_signatures.size() &&
                module_.region_signatures[fresh].return_region !=
                    MIR_INVALID_ID) {
                func.region_signature = fresh;
                module_.set_symbol_region_signature(func.symbol, fresh);
            }
        }
    }
    if (func.region_signature == MIR_INVALID_ID) {
        func.region_signature = intern_region_signature(decl);
        if (func.region_signature != MIR_INVALID_ID) {
            module_.set_symbol_region_signature(func.symbol, func.region_signature);
        }
    }
    if (func.region_signature != MIR_INVALID_ID &&
        func.region_signature < module_.region_signatures.size()) {
        func.region_var_count =
            module_.region_signatures[func.region_signature].lifetime_param_count;
    }
    func.function_type = module_.symbols.get(func.symbol).type;

    MIRBuilder b(arena, module_, func);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    func.entry_block = entry;

    const TypeId ret = func.function_type < module_.types.size()
                           ? module_.types.get(func.function_type).element
                           : MIR_INVALID_ID;
    if (ret == MIR_INVALID_ID || ret >= module_.types.size()) {
        b.unreachable();
    } else {
        const MIRTypeRecord& r = module_.types.get(ret);
        switch (r.kind) {
            case MIRTypeKind::Void:
                b.ret_void();
                break;
            case MIRTypeKind::Bool:
                b.ret(b.const_bool(ret, module_.constants.add_bool(ret, false)));
                break;
            case MIRTypeKind::Int:
                b.ret(b.const_int(ret, module_.constants.add_int(ret, 0)));
                break;
            case MIRTypeKind::Float:
                // an sret return is void above; a scalar gets a zero. The
                // emitter coerces the constant to the declared return type.
                if (r.size == 4) b.ret(b.const_float(ret, module_.constants.add_float(ret, 0.0f)));
                else if (r.size == 8) b.ret(b.const_double(ret, module_.constants.add_double(ret, 0.0)));
                else b.unreachable();
                break;
            case MIRTypeKind::Pointer:
            case MIRTypeKind::Reference:
            case MIRTypeKind::Function:
                b.ret(b.const_null(ret, module_.constants.add_null(ret)));
                break;
            default:
                // an aggregate passed back by pointer has no meaningful zero;
                // the C backend's stub is an empty body here as well
                b.unreachable();
                break;
        }
    }
    if (!b.ok()) {
        error = b.error() ? b.error() : "failed to build the stub body";
        return false;
    }
    func.flags |= MF_WEAK_STUB;
    return true;
}

bool MIRLowerer::lower_function(FunctionDeclaration* decl, MIRArena& arena,
                                MIRFunction& func, std::string& error) {
    MIR_OP(functions);
#ifdef MIR_OPS_COUNTERS
    struct LowerTimer {
        std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
        ~LowerTimer() {
            g_hot_ops.lower_ns += static_cast<unsigned long long>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - t0).count());
        }
    } lower_timer;
#endif
    if (decl->is_async()) {
        return lower_async_function(decl, arena, func, error);
    }
    // Cache the "constructor writes the whole result" proof now, while this
    // function's body (which this module owns) is still alive, so a dependent
    // module can consume it without reading a freed body (see
    // construction_writes_whole_value).
    prime_constructor_fact(decl);
    arena_ = &arena;
    current_func_ = decl;
    current_fn_is_comptime_ = decl->is_comptime();
    if (set_current_func_) set_current_func_(decl);
    var_places_.clear();
    name_places_.clear();
    aggregate_param_ptrs_.clear();
    destructibles_.clear();
    scope_places_.clear();
    stmt_temps_.clear();
    // func_symbols_ persists across functions in a module so callees keep stable ids

    func.symbol = intern_function(decl);
    if (source_loc_provider_ && decl) {
        const uint64_t enc = decl->encoded_location().encoded;
        if (enc != 0 && source_loc_provider_(enc, func.decl_loc)) {
            func.has_decl_loc = true;
        }
    }
    // Region signatures are built from the declaration before any body
    // lowering, so MIRBorrowCheck (M4) never has to read the AST.
    func.region_signature = module_.symbol_region_signature(func.symbol);
    // A generic declaration is interned before its return type is linked, so a
    // cached signature can lack the return region. Rebuild from the linked
    // declaration when that happened (S4.6a aggregate returns).
    if (func.region_signature != MIR_INVALID_ID &&
        func.region_signature < module_.region_signatures.size() &&
        module_.region_signatures[func.region_signature].return_region ==
            MIR_INVALID_ID &&
        decl->returnType != nullptr) {
        if (declaration_has_return_lifetime(decl->returnType)) {
            const uint32_t fresh = intern_region_signature(decl);
            if (fresh != MIR_INVALID_ID &&
                fresh < module_.region_signatures.size() &&
                module_.region_signatures[fresh].return_region !=
                    MIR_INVALID_ID) {
                func.region_signature = fresh;
                module_.set_symbol_region_signature(func.symbol, fresh);
            }
        }
    }
    if (func.region_signature == MIR_INVALID_ID) {
        func.region_signature = intern_region_signature(decl);
        if (func.region_signature != MIR_INVALID_ID) {
            module_.set_symbol_region_signature(func.symbol, func.region_signature);
        }
    }
    if (func.region_signature != MIR_INVALID_ID &&
        func.region_signature < module_.region_signatures.size()) {
        func.region_var_count =
            module_.region_signatures[func.region_signature].lifetime_param_count;
    }
    const TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    sret_ = abi::needs_sret(module_, ret);
    sret_ret_type_ = ret;
    return_type_ = decl->returnType;
    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
    current_self_param_ = self_param;
    // a static interface impl method uses the interface's erased `void**` self ABI
    FunctionDeclaration* static_base = lower_static_iface_base_method(decl);
    FunctionParam* static_base_self = static_base ? static_base->get_self_param() : nullptr;
    destructor_def_ = decl->is_delete_fn() && decl->parent()
                          ? decl->parent()->as_extendable_member_container()
                          : nullptr;
    bool self_in_params = false;
    for (FunctionParam* p : decl->params) {
        if (p == self_param) { self_in_params = true; break; }
    }
    if (self_in_params) self_param = nullptr; // already bound by the params loop
    {
        MIRScratch<TypeId> _ptypes(*this);
        std::vector<TypeId>& ptypes = _ptypes.v;
        if (sret_) {
            sret_ptr_type_ = types_.pointer_type(ret, true);
            ptypes.push_back(sret_ptr_type_);
        }
        if (self_param) {
            FunctionDeclaration* base = lower_static_iface_base_method(decl);
            FunctionParam* base_self = base ? base->get_self_param() : nullptr;
            TypeId pt;
            if (base_self && base_self->type) {
                pt = types_.map(base_self->type);
            } else {
                pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
                if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
                    pt = types_.pointer_type(module_.types.get(pt).element, true);
                } else if (abi::is_aggregate(module_, pt)) {
                    pt = types_.pointer_type(pt, true);
                }
            }
            ptypes.push_back(pt);
        }
        for (FunctionParam* p : decl->params) {
            if (p == current_self_param_ && static_base_self && static_base_self->type) {
                ptypes.push_back(types_.map(static_base_self->type));
                continue;
            }
            TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
            ptypes.push_back(pt);
        }
        // a variadic function's last parameter is the tail marker, spelled `...`
        if (decl->isVariadic() && !decl->params.empty()) {
            ptypes.pop_back();
        }
        const TypeId ftype = types_.function_signature(sret_ ? types_.void_type() : ret, ptypes,
                                                      decl->isVariadic());
        module_.symbols.set_type(func.symbol, ftype);
        func.function_type = ftype;
    }

    MIRBuilder b(arena, module_, func);
    builder_ = &b;

    // attach the declaration's source location (a DISubprogram and the default
    // debug location for prologue instructions; statements refine it)
    {
        MIRSourceLoc dloc;
        const uint64_t enc = decl->encoded_location().encoded;
        if (source_loc_provider_ && enc != 0 && source_loc_provider_(enc, dloc)) {
            b.set_function_source(dloc);
            b.set_source(b.add_source(dloc));
        }
    }

    const BlockId entry = b.create_block();
    b.set_block(entry);
    func.entry_block = entry;

    if (sret_) {
        sret_ptr_ = b.param(sret_ptr_type_);
    }

    if (self_param) {
        TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (abi::is_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(self_param, place);
        bind_name(self_param->name, place);
    }

    // parameters: materialize an SSA param value, spill to a place, bind it
    const size_t bind_param_count = (decl->isVariadic() && !decl->params.empty())
        ? decl->params.size() - 1 : decl->params.size();
    for (size_t pi = 0; pi < bind_param_count; ++pi) {
        FunctionParam* p = decl->params[pi];
        const TypeId declared = p && p->type ? types_.map(p->type) : types_.opaque_type();
        const bool is_array = declared != MIR_INVALID_ID &&
                              module_.types.get(declared).kind == MIRTypeKind::Array;
        const bool is_aggregate = declared != MIR_INVALID_ID && !is_array &&
                                  !(module_.types.get(declared).flags & TF_TYPEDEF) &&
                                  abi::is_aggregate(module_, declared);
        TypeId pt = declared;
        if (is_array) {
            pt = types_.pointer_type(module_.types.get(declared).element, true);
        } else if (is_aggregate) {
            pt = types_.pointer_type(declared, true);
        }
        // the self param of a static interface impl method is `void**` on the
        // ABI; cast it to the real receiver type before binding
        const bool static_self = p == current_self_param_ && static_base_self &&
                                 static_base_self->type;
        const TypeId abi_pt = static_self ? types_.map(static_base_self->type) : pt;
        ValueId pv = b.param(abi_pt);
        if (static_self && abi_pt != pt) pv = b.cast(pv, pt);
        PlaceId place;
        if (is_aggregate) {
            // the aggregate is passed by a hidden pointer; materialize a
            // by-value copy so the body sees the value, not the pointer
            place = b.alloca(declared, MIRStorageClass::Parameter);
            ValueId val = b.load_indirect(pv, declared);
            b.store(place, val);
            // remember the argument pointer for escaping addresses (`dyn<T>(p)`)
            aggregate_param_ptrs_[p] = pv;
        } else {
            place = b.alloca(pt, MIRStorageClass::Parameter);
            b.store(place, pv);
        }
        if (place == MIR_INVALID_ID) {
            error = "failed to allocate parameter place";
            builder_ = nullptr;
            return false;
        }
        builder_->function().places[place].param_index =
            static_cast<uint32_t>(pi);
        bind(p, place);
        bind_name(p->name, place);
        // by-value struct params own their value and are destroyed at exit
        if (p->type) {
            const TypeId ptt = types_.map(p->type);
            const MIRTypeRecord& prr = module_.types.get(ptt);
            if (!(prr.flags & TF_TYPEDEF) && abi::is_aggregate(module_, ptt)) {
                register_destructible(place, p->type);
            }
        }
    }

    if (decl->is_generated_fn() && decl->is_constructor_fn()) {
        // a generated constructor must default-initialize the fields (and
        // inherited bases); the body itself is empty. Its receiver is the sret
        // `this` pointer (`struct T* this`) or the self parameter.
        ExtendableMembersContainerNode* cdef =
            decl->parent() ? decl->parent()->as_extendable_member_container() : nullptr;
        ValueId recv = MIR_NULL;
        bool have_recv = false;
        if (sret_) {
            recv = sret_ptr_;
            have_recv = true;
        } else if (self_param) {
            PlaceId selfp = place_for_linked(self_param);
            if (selfp == MIR_INVALID_ID) selfp = place_for_name(self_param->name);
            if (selfp != MIR_INVALID_ID) {
                const TypeId st = b.function().places[selfp].type;
                const MIRTypeRecord& sr = module_.types.get(st);
                recv = (sr.kind == MIRTypeKind::Pointer || sr.kind == MIRTypeKind::Reference)
                           ? b.load(selfp, st)
                           : b.address_of(selfp, types_.pointer_type(st, true));
                have_recv = true;
            }
        }
        if (cdef && have_recv) {
            if (!apply_struct_defaults_ptr(recv, cdef, error)) {
                builder_ = nullptr;
                return false;
            }
        }
    }

    if (decl->body.has_value()) {
        for (ASTNode* node : decl->body->nodes) {
            const size_t smark = stmt_temps_.size();
            if (!lower_stmt(node, error)) {
                builder_ = nullptr;
                return false;
            }
            if (!b.current_block_terminated()) flush_stmt_temps(smark);
            else stmt_temps_.resize(smark);
        }
    }

    if (!b.current_block_terminated()) {
        if (destructor_def_) emit_destructor_members(error);
        emit_drops();
        b.ret_void();
    }

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "MIR builder failure";
        return false;
    }
    return true;
}

} // namespace mir
