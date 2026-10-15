// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"
#include "MIRLowererInternal.h"
#include "MIRABI.h"
#include "MIRPrototype.h"

#include <cstdlib>
#include <iostream>
#include <optional>

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
#include "ast/values/RuntimeValue.h"
#include "ast/values/DynamicValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/structures/CapturedComptimeVariable.h"
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
#include "ast/values/RuntimeBlockValue.h"
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

namespace {

/**
 * `sizeof(x)` / `alignof(x)` / `offsetof(x, m)` parse `x` as a named type, so
 * the operand may be a `LinkedType` pointing at a variable, parameter or member
 * rather than a type declaration. Resolve those to the declared type so the
 * size/align/offset uses the real type instead of an opaque pointer.
 *
 * Without this, `sizeof(buffer)` for `var buffer : [160]char` mapped to
 * `opaque_type()` and emitted `sizeof(void*)` (8), silently truncating any
 * caller that formatted into such a buffer (the test runner used one for the
 * spawned test id, so every suite reported success without running anything).
 */
/**
 * True when the Chemical struct `derived` inherits (directly or transitively)
 * from `base`. Used to decide whether a pointer receiver of a derived type can
 * be passed straight through to a method expecting a base-type pointer.
 */
bool struct_derives_from(StructDefinition* derived, BaseType* base) {
    if (!derived || !base) return false;
    BaseType* base_canon = base->canonical();
    for (auto& inh : derived->inherited) {
        BaseType* bt = const_cast<BaseType*>(inh.type.getType());
        if (!bt) continue;
        if (bt->canonical() == base_canon) return true;
        ASTNode* bn = bt->get_direct_linked_canonical_node();
        if (bn && bn->kind() == ASTNodeKind::StructDecl) {
            if (struct_derives_from(bn->as_struct_def_unsafe(), base)) return true;
        }
    }
    return false;
}

BaseType* resolve_sizeof_operand(BaseType* t) {
    for (int hops = 0; t && hops < 16; ++hops) {
        if (t->kind() != BaseTypeKind::Linked) return t;
        ASTNode* n = t->get_direct_linked_node();
        if (!n) return t;
        switch (n->kind()) {
            case ASTNodeKind::VarInitStmt:
                t = n->as_var_init_unsafe()->known_type();
                break;
            case ASTNodeKind::FunctionParam:
                t = const_cast<BaseType*>(n->as_func_param_unsafe()->type.getType());
                break;
            case ASTNodeKind::StructMember:
                t = const_cast<BaseType*>(n->as_struct_member_unsafe()->type.getType());
                break;
            default:
                return t;
        }
    }
    return t;
}

/** Resolve a captured-comptime bridge node to its evaluated value, if any. */
Value* captured_comptime_value(ASTNode* linked) {
    if (!linked || linked->kind() != ASTNodeKind::CapturedComptimeVariable) return nullptr;
    auto* ccv = static_cast<CapturedComptimeVariable*>(linked);
    if (!ccv->parent_refs || ccv->index >= ccv->parent_refs->refs.size()) return nullptr;
    return ccv->parent_refs->refs[ccv->index].evaluated;
}

// The ABI classification (`needs_aggregate_path` / `needs_sret_return` /
// `is_struct_like`) now lives in `MIRABI.h`, shared verbatim with every emitter:
// see `abi::is_aggregate`, `abi::needs_sret`, `abi::is_struct_like`.

/**
 * Resolve a member (which may be inherited) to its dotted C field path from
 * `container`, e.g. `WalkingAnimal.Animal.a`. The C layout nests each base
 * struct as a field named after the base type.
 */
bool inherited_field_path(ASTNode* container, ASTNode* member, std::string& out) {
    if (!container || !member) return false;
    if (member->parent() == container) {
        auto* bdm = member->as_base_def_member();
        if (!bdm) return false;
        out.assign(bdm->name.data(), bdm->name.size());
        return true;
    }
    auto* ec = container->as_extendable_member_container();
    if (!ec) return false;
    for (InheritedType& inh : ec->inherited) {
        BaseType* bt = inh.type;
        ASTNode* base = bt ? bt->get_direct_linked_canonical_node() : nullptr;
        if (!base) continue;
        std::string sub;
        if (inherited_field_path(base, member, sub)) {
            auto* bec = base->as_extendable_member_container();
            const chem::string_view bn = bec ? bec->name_view() : chem::string_view();
            if (bn.empty()) return false;
            out.assign(bn.data(), bn.size());
            out += ".";
            out += sub;
            return true;
        }
    }
    return false;
}

// An unqualified field access inside a method resolves through `self`. The
// linked node is usually the `StructMember`, but an anonymous aggregate field
// (an inline `struct`/`union`) is linked to its `UnnamedStruct`/`UnnamedUnion`
// type node instead.
static bool is_implicit_field_node(ASTNode* n) {
    if (!n) return false;
    return n->kind() == ASTNodeKind::StructMember ||
           n->kind() == ASTNodeKind::UnnamedStruct ||
           n->kind() == ASTNodeKind::UnnamedUnion;
}

bool map_binary(Operation op, MIROpcode& out, MIRBinaryOp& bop) {    switch (op) {
        case Operation::Addition: out = MIROpcode::Binary; bop = MIRBinaryOp::Add; return true;
        case Operation::Subtraction: out = MIROpcode::Binary; bop = MIRBinaryOp::Sub; return true;
        case Operation::Multiplication: out = MIROpcode::Binary; bop = MIRBinaryOp::Mul; return true;
        case Operation::Division: out = MIROpcode::Binary; bop = MIRBinaryOp::Div; return true;
        case Operation::Modulus: out = MIROpcode::Binary; bop = MIRBinaryOp::Rem; return true;
        case Operation::LeftShift: out = MIROpcode::Binary; bop = MIRBinaryOp::Shl; return true;
        case Operation::RightShift: out = MIROpcode::Binary; bop = MIRBinaryOp::Shr; return true;
        case Operation::BitwiseAND: out = MIROpcode::Binary; bop = MIRBinaryOp::BitAnd; return true;
        case Operation::BitwiseOR: out = MIROpcode::Binary; bop = MIRBinaryOp::BitOr; return true;
        case Operation::BitwiseXOR: out = MIROpcode::Binary; bop = MIRBinaryOp::BitXor; return true;
        case Operation::GreaterThan: out = MIROpcode::Compare; bop = MIRBinaryOp::Gt; return true;
        case Operation::GreaterThanOrEqual: out = MIROpcode::Compare; bop = MIRBinaryOp::Ge; return true;
        case Operation::LessThan: out = MIROpcode::Compare; bop = MIRBinaryOp::Lt; return true;
        case Operation::LessThanOrEqual: out = MIROpcode::Compare; bop = MIRBinaryOp::Le; return true;
        case Operation::IsEqual: out = MIROpcode::Compare; bop = MIRBinaryOp::Eq; return true;
        case Operation::IsNotEqual: out = MIROpcode::Compare; bop = MIRBinaryOp::Ne; return true;
        default: return false;
    }
}

// the `core::ops` interface method for an overloadable operator (or nullptr)
const char* operator_method_name(Operation op) {
    switch (op) {
        case Operation::Addition: return "add";
        case Operation::Subtraction: return "sub";
        case Operation::Multiplication: return "mul";
        case Operation::Division: return "div";
        case Operation::Modulus: return "rem";
        case Operation::LeftShift: return "shl";
        case Operation::RightShift: return "shr";
        case Operation::BitwiseAND: return "bitand";
        case Operation::BitwiseOR: return "bitor";
        case Operation::BitwiseXOR: return "bitxor";
        case Operation::GreaterThan: return "gt";
        case Operation::GreaterThanOrEqual: return "gte";
        case Operation::LessThan: return "lt";
        case Operation::LessThanOrEqual: return "lte";
        case Operation::IsEqual: return "eq";
        case Operation::IsNotEqual: return "ne";
        default: return nullptr;
    }
}

// the `core::ops` interface method for an overloadable compound assignment
const char* compound_method_name(Operation op) {
    switch (op) {
        case Operation::AddTo:
        case Operation::Addition: return "add_assign";
        case Operation::SubtractFrom:
        case Operation::Subtraction: return "sub_assign";
        case Operation::MultiplyBy:
        case Operation::Multiplication: return "mul_assign";
        case Operation::DivideBy:
        case Operation::Division: return "div_assign";
        case Operation::ModuloBy:
        case Operation::Modulus: return "rem_assign";
        case Operation::ShiftLeftBy:
        case Operation::LeftShift: return "shl_assign";
        case Operation::ShiftRightBy:
        case Operation::RightShift: return "shr_assign";
        case Operation::ANDWith:
        case Operation::BitwiseAND: return "bitand_assign";
        case Operation::ExclusiveORWith:
        case Operation::BitwiseXOR: return "bitxor_assign";
        case Operation::InclusiveORWith:
        case Operation::BitwiseOR: return "bitor_assign";
        default: return nullptr;
    }
}

bool compound_binary(Operation op, MIRBinaryOp& out) {
    switch (op) {
        case Operation::AddTo: out = MIRBinaryOp::Add; return true;
        case Operation::SubtractFrom: out = MIRBinaryOp::Sub; return true;
        case Operation::MultiplyBy: out = MIRBinaryOp::Mul; return true;
        case Operation::DivideBy: out = MIRBinaryOp::Div; return true;
        case Operation::ModuloBy: out = MIRBinaryOp::Rem; return true;
        case Operation::ShiftLeftBy: out = MIRBinaryOp::Shl; return true;
        case Operation::ShiftRightBy: out = MIRBinaryOp::Shr; return true;
        case Operation::ANDWith: out = MIRBinaryOp::BitAnd; return true;
        case Operation::ExclusiveORWith: out = MIRBinaryOp::BitXor; return true;
        case Operation::InclusiveORWith: out = MIRBinaryOp::BitOr; return true;
        // the parser records `a += b` as Addition (not AddTo): accept both
        case Operation::Addition: out = MIRBinaryOp::Add; return true;
        case Operation::Subtraction: out = MIRBinaryOp::Sub; return true;
        case Operation::Multiplication: out = MIRBinaryOp::Mul; return true;
        case Operation::Division: out = MIRBinaryOp::Div; return true;
        case Operation::Modulus: out = MIRBinaryOp::Rem; return true;
        case Operation::LeftShift: out = MIRBinaryOp::Shl; return true;
        case Operation::RightShift: out = MIRBinaryOp::Shr; return true;
        case Operation::BitwiseAND: out = MIRBinaryOp::BitAnd; return true;
        case Operation::BitwiseXOR: out = MIRBinaryOp::BitXor; return true;
        case Operation::BitwiseOR: out = MIRBinaryOp::BitOr; return true;
        default: return false;
    }
}

} // namespace

PlaceId MIRLowerer::place_for_linked(ASTNode* linked) const {
    if (!linked) return MIR_INVALID_ID;
    auto it = var_places_.find(linked);
    if (it == var_places_.end()) return MIR_INVALID_ID;
    return it->second;
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
    const auto prev = iface->active_user;
    iface->active_user = user;
    std::string name = mangler_ ? mangler_(decl) : "";
    iface->active_user = prev;
    if (name.empty()) return intern_function(decl);

    auto it = named_method_symbols_.find(name);
    if (it != named_method_symbols_.end()) return it->second;

    TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    std::vector<TypeId> ptypes;
    ptypes.reserve(decl->params.size());
    for (FunctionParam* p : decl->params) {
        ptypes.push_back(p && p->type ? types_.map(p->type) : types_.opaque_type());
    }
    TypeId ftype = types_.function_signature(ret, ptypes, decl->isVariadic());
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = (decl->is_extern() || decl->is_linkage_public()) ? MIRLinkage::External
                                                                  : MIRLinkage::Internal;
    rec.type = ftype;
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    named_method_symbols_[name] = s;
    return s;
}

MIRExprResult MIRLowerer::lower_address_of(Value* inner, std::string& error) {
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
        if (Value* cv = captured_comptime_value(id->linked)) {
            return lower_expr(cv, error);
        }
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

bool MIRLowerer::member_name(Value* v, std::string& out, std::string& error) {
    Value* t = v;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t || t->val_kind() != ValueKind::Identifier) {
        error = "member is not an identifier";
        return false;
    }
    const chem::string_view name = t->as_identifier_unsafe()->value;
    out.assign(name.data(), name.size());
    return true;
}

bool MIRLowerer::member_field_path(Value* base, Value* member, std::string& out,
                                   std::string& error) {
    Value* m = member;
    if (m && m->val_kind() == ValueKind::AccessChain) {
        auto* c = m->as_access_chain_unsafe();
        if (c->values.size() == 1) m = c->values[0];
    }
    ASTNode* mnode = m ? m->linked_node() : nullptr;
    BaseType* bt = base ? base->getType() : nullptr;
    // unwrap a pointer/reference base (`&self` / `self` by pointer) so the
    // container node is reachable for inherited-field resolution
    if (bt) {
        BaseType* bc = bt->canonical();
        if (bc && bc->kind() == BaseTypeKind::Pointer) {
            bt = bc->as_pointer_type_unsafe()->type;
        } else if (bc && bc->kind() == BaseTypeKind::Reference) {
            bt = bc->as_reference_type_unsafe()->type;
        }
    }
    ASTNode* cn = bt ? bt->get_direct_linked_canonical_node() : nullptr;
    if (mnode && cn) {
        std::string path;
        if (inherited_field_path(cn, mnode, path)) {
            out = path;
            return true;
        }
    }
    return member_name(member, out, error);
}

FunctionParam* MIRLowerer::aggregate_param_operand(Value* v) {
    if (aggregate_param_ptrs_.empty()) return nullptr;
    Value* t = v;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() != 1) return nullptr;
        t = c->values[0];
    }
    if (!t || t->val_kind() != ValueKind::Identifier) return nullptr;
    ASTNode* linked = t->linked_node();
    if (!linked || linked->kind() != ASTNodeKind::FunctionParam) return nullptr;
    auto* fp = linked->as_func_param_unsafe();
    auto it = aggregate_param_ptrs_.find(fp);
    return it == aggregate_param_ptrs_.end() ? nullptr : fp;
}

bool MIRLowerer::lower_call_args(const std::vector<Value*>& values,
                                 std::vector<MIROperand>& args, std::string& error) {
    return lower_call_args_for(nullptr, false, values, args, error);
}

bool MIRLowerer::lower_call_args_for(FunctionDeclaration* fd, bool self_included,
                                     const std::vector<Value*>& values,
                                     std::vector<MIROperand>& args, std::string& error,
                                     const std::vector<BaseType*>* extra_params) {
    const size_t offset = self_included ? 1 : 0;
    for (size_t i = 0; i < values.size(); ++i) {
        BaseType* param_type = nullptr;
        if (fd && i + offset < fd->params.size()) {
            param_type = fd->params[i + offset]->type;
        } else if (!fd && extra_params && i + offset < extra_params->size()) {
            param_type = (*extra_params)[i + offset];
        }
        MIRExprResult r = lower_arg_converted(values[i], param_type, error);
        if (!r.ok()) return false;
        // a non-pointer value passed to a reference parameter must be
        // materialized in a temporary whose address is passed
        if (param_type && param_type->kind() == BaseTypeKind::Reference &&
            r.kind == MIRExprKind::Value && r.type != MIR_INVALID_ID &&
            r.type < module_.types.size()) {
            const MIRTypeKind rk = module_.types.get(r.type).kind;
            if (rk != MIRTypeKind::Pointer && rk != MIRTypeKind::Reference) {
                BaseType* inner = param_type->as_reference_type()->type;
                const TypeId it = types_.map(inner);
                const PlaceId tmp = builder_->alloca(it, MIRStorageClass::Temporary);
                builder_->store(tmp, r.id);
                const TypeId ptrt = types_.pointer_type(it, true);
                r = MIRExprResult::value(builder_->address_of(tmp, ptrt), ptrt);
            }
        }
        // track destructible temporaries passed by value (destroyed after the call)
        if (r.kind == MIRExprKind::Place && values[i]) {
            Value* v0 = values[i];
            if (v0->val_kind() == ValueKind::AccessChain) {
                auto* c = v0->as_access_chain_unsafe();
                if (c->values.size() == 1) v0 = c->values[0];
            }
            const ValueKind vk = v0->val_kind();
            const bool by_value =
                !(param_type && (param_type->kind() == BaseTypeKind::Reference ||
                                 param_type->kind() == BaseTypeKind::Pointer));
            const bool known_param = param_type != nullptr;
            if (vk == ValueKind::StructValue || vk == ValueKind::FunctionCall) {
                BaseType* at = v0->getType();
                // when the parameter type is unknown (an indirect/function-pointer
                // call) conservatively treat a temporary as caller-owned
                if (at && (!by_value || !known_param)) {
                    call_temps_.emplace_back(static_cast<PlaceId>(r.id), at);
                }
            } else if (vk == ValueKind::Identifier && by_value) {
                // a local passed by value is moved out; an array decays to a
                // pointer and stays owned by the caller
                BaseType* at = v0->getType();
                BaseType* canon = at ? at->canonical() : nullptr;
                if (!(canon && canon->kind() == BaseTypeKind::Array)) {
                    mark_moved(static_cast<PlaceId>(r.id));
                }
            }
        }
        if (!push_call_arg(r, args, error)) return false;
    }
    return true;
}

void MIRLowerer::destroy_call_temps(std::string& error) {
    // A temporary passed by reference (`&T`) is owned by the caller and must be
    // destroyed after the call; by-value temporaries are owned by the callee's
    // parameter instead.
    for (auto& [place, type] : call_temps_) {
        const SymbolId dtor = destructor_symbol(type);
        if (dtor == MIR_INVALID_ID) continue;
        builder_->destroy(place, dtor);
    }
    call_temps_.clear();
    (void)error;
}

void MIRLowerer::destroy_call_temps_from(size_t mark, std::string& error) {
    // destroy only the temporaries created for the call that started at `mark`,
    // leaving the enclosing call's in-progress temporaries intact
    for (size_t i = mark; i < call_temps_.size(); ++i) {
        const SymbolId dtor = destructor_symbol(call_temps_[i].second);
        if (dtor == MIR_INVALID_ID) continue;
        builder_->destroy(call_temps_[i].first, dtor);
    }
    call_temps_.resize(mark);
    (void)error;
}

SymbolId MIRLowerer::destructor_symbol(BaseType* type) {
    if (!type) return MIR_INVALID_ID;
    BaseType* canon = type->canonical();
    if (!canon) return MIR_INVALID_ID;
    if (canon->kind() == BaseTypeKind::Array) {
        // an array of destructibles drops each element
        return destructor_symbol(canon->as_array_type_unsafe()->elem_type);
    }
    ASTNode* node = canon->get_direct_linked_canonical_node();
    if (!node) return MIR_INVALID_ID;
    FunctionDeclaration* dtor = nullptr;
    if (node->kind() == ASTNodeKind::StructDecl) {
        dtor = node->as_struct_def_unsafe()->destructor_func();
    } else if (node->kind() == ASTNodeKind::VariantDecl) {
        dtor = node->as_variant_def_unsafe()->destructor_func();
    } else if (node->kind() == ASTNodeKind::UnionDecl) {
        dtor = node->as_union_def_unsafe()->destructor_func();
    }
    if (!dtor || !dtor->body.has_value()) return MIR_INVALID_ID;
    return intern_function(dtor);
}

SymbolId MIRLowerer::extern_symbol(const char* name, TypeId ftype) {
    auto it = extern_symbols_.find(name);
    if (it != extern_symbols_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::External;
    rec.type = ftype;
    const uint32_t len = static_cast<uint32_t>(std::strlen(name));
    SymbolId s = module_.symbols.add(rec, name, len, name, len);
    extern_symbols_[name] = s;
    return s;
}

ValueId MIRLowerer::emit_malloc(TypeId size_v, TypeId u64t, TypeId ptr_t) {
    const TypeId ft = types_.function_signature(ptr_t, {u64t});
    const SymbolId sym = extern_symbol("malloc", ft);
    MIROperand arg = MIROperand::value(size_v, u64t);
    return builder_->call_scalar(sym, ptr_t, &arg, 1);
}

void MIRLowerer::emit_free(ValueId ptr) {
    const TypeId vpt = types_.pointer_type(types_.void_type(), true);
    const TypeId ft = types_.function_signature(types_.void_type(), {vpt});
    const SymbolId sym = extern_symbol("free", ft);
    const ValueId p = (ptr != MIR_NULL) ? builder_->cast(ptr, vpt) : ptr;
    MIROperand arg = MIROperand::value(p, vpt);
    builder_->call_scalar(sym, types_.void_type(), &arg, 1);
}

void MIRLowerer::emit_member_dtor(ValueId self_ptr, const chem::string_view& member,
                                  BaseType* mt, std::string& error) {
    if (!mt) return;
    BaseType* mc = mt->canonical();
    if (!mc) return;
    if (mc->kind() == BaseTypeKind::CapturingFunction) {
        // a `std::function` field owns a closure instance; destroy that
        BaseType* inst = const_cast<BaseType*>(
            mc->as_capturing_func_type_unsafe()->instance_type.getType());
        emit_member_dtor(self_ptr, member, inst, error);
        return;
    }
    if (mc->kind() == BaseTypeKind::Array) {
        BaseType* elem = mc->as_array_type_unsafe()->elem_type;
        const SymbolId dtor = destructor_symbol(elem);
        if (dtor == MIR_INVALID_ID) return;
        const uint64_t n = mc->as_array_type_unsafe()->get_array_size();
        if (n == 0) return;
        const TypeId ft = types_.map(mt);
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, member.data(), static_cast<uint32_t>(member.size()));
        const TypeId eptr = types_.pointer_type(types_.map(elem), true);
        // `field_addr_ptr` spells an array field as a pointer-to-array
        // (`Elem (*v)[N]`); cast it to an element pointer for the loop
        const ValueId faddr =
            builder_->cast(builder_->field_addr_ptr(self_ptr, fc, ft), eptr);
        const TypeId u64t = types_.u64_type();
        const ConstantId add_op = module_.constants.add_int(
            MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Add));
        for (uint64_t i = 0; i < n; ++i) {
            const ValueId off = builder_->const_int(u64t, module_.constants.add_int(u64t, i));
            const ValueId ep = (i == 0) ? faddr : builder_->binary(faddr, off, add_op, eptr);
            MIROperand a = MIROperand::value(ep, eptr);
            builder_->call_scalar(dtor, types_.void_type(), &a, 1);
        }
        return;
    }
    const SymbolId dtor = destructor_symbol(mt);
    if (dtor == MIR_INVALID_ID) return;
    const TypeId ft = types_.map(mt);
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, member.data(), static_cast<uint32_t>(member.size()));
    const ValueId faddr = builder_->field_addr_ptr(self_ptr, fc, ft);
    MIROperand a = MIROperand::value(faddr, types_.pointer_type(ft, true));
    builder_->call_scalar(dtor, types_.void_type(), &a, 1);
}

void MIRLowerer::emit_destructor_members(std::string& error) {
    if (!destructor_def_ || !current_self_param_) return;
    PlaceId selfp = place_for_linked(current_self_param_);
    if (selfp == MIR_INVALID_ID) selfp = place_for_name(current_self_param_->name);
    if (selfp == MIR_INVALID_ID) return;
    const TypeId st = builder_->function().places[selfp].type;
    const MIRTypeRecord& sr = module_.types.get(st);
    ValueId self_ptr = MIR_NULL;
    if (sr.kind == MIRTypeKind::Pointer || sr.kind == MIRTypeKind::Reference) {
        self_ptr = builder_->load(selfp, st);
    } else {
        self_ptr = builder_->address_of(selfp, types_.pointer_type(st, true));
    }
    if (destructor_def_->kind() == ASTNodeKind::VariantDecl) {
        emit_variant_destructor_members(destructor_def_->as_variant_def_unsafe(), self_ptr, error);
        return;
    }
    for (auto& inh : destructor_def_->inherited) {
        auto* linked = inh.type ? inh.type->get_direct_linked_struct() : nullptr;
        if (linked) {
            emit_member_dtor(self_ptr, linked->name_view(), inh.type, error);
        }
    }
    for (auto* var : destructor_def_->variables()) {
        emit_member_dtor(self_ptr, var->name, var->known_type(), error);
    }
}

void MIRLowerer::emit_variant_destructor_members(VariantDefinition* vd, ValueId self_ptr,
                                                 std::string& error) {
    if (!vd) return;
    auto& vars = vd->variables();
    if (vars.empty()) return;
    const TypeId tag_t = types_.u32_type();
    const ConstantId tag_fc =
        module_.constants.add_string(MIR_INVALID_ID, "__chx__vt_621827", 16);
    const ValueId tag = builder_->field_load_ptr(self_ptr, tag_fc, tag_t);
    const ConstantId eq_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));
    const BlockId done = builder_->create_block();
    unsigned index = 0;
    for (auto* var : vars) {
        auto* member = var ? var->as_variant_member_unsafe() : nullptr;
        if (!member) {
            index++;
            continue;
        }
        const BlockId next = builder_->create_block();
        const BlockId match = builder_->create_block();
        const ValueId idx_v =
            builder_->const_int(tag_t, module_.constants.add_int(tag_t, index));
        const ValueId is_eq = builder_->compare(tag, idx_v, eq_op, types_.bool_type());
        builder_->cond_br(is_eq, match, next);
        builder_->set_block(match);
        for (auto& p : member->values) {
            auto* param = p.second;
            if (!param) continue;
            std::string path(member->name.data(), member->name.size());
            path += ".";
            path += std::string(param->name.data(), param->name.size());
            emit_member_dtor(self_ptr, chem::string_view(path.data(), path.size()), param->type,
                             error);
        }
        if (!builder_->current_block_terminated()) builder_->br(done);
        builder_->set_block(next);
        index++;
    }
    if (!builder_->current_block_terminated()) builder_->br(done);
    builder_->set_block(done);
}

void MIRLowerer::register_destructible(PlaceId place, BaseType* type) {
    const SymbolId dtor = destructor_symbol(type);
    if (dtor == MIR_INVALID_ID) return;
    PlaceId flag = MIR_INVALID_ID;
    if (async_ && place < builder_->function().places.size()) {
        // a frame-resident destructible uses the frame drop flag the drop
        // function checks, so completion and cancellation agree on liveness
        const MIRPlaceDef& pd = builder_->function().places[place];
        if (pd.storage_class == static_cast<uint8_t>(MIRStorageClass::FrameField) &&
            pd.frame_field < module_.constants.size()) {
            const MIRConstant& c = module_.constants.get(pd.frame_field);
            std::string fname(module_.constants.data.data() + c.data_offset, c.data_count);
            const std::string prefix = "__chx_slot_";
            if (fname.rfind(prefix, 0) == 0) {
                const std::string df = "__chx_drop_" + fname.substr(prefix.size());
                const ConstantId dfc = module_.constants.add_string(
                    MIR_INVALID_ID, df.data(), static_cast<uint32_t>(df.size()));
                flag = builder_->alloca_frame(types_.bool_type(), dfc);
            }
        }
    }
    if (flag == MIR_INVALID_ID) {
        flag = builder_->alloca(types_.bool_type(), MIRStorageClass::Local);
    }
    builder_->set_drop(flag, true);
    destructibles_.push_back({place, flag, dtor});
}

void MIRLowerer::maybe_register_call_temp(Value* v, MIRExprResult r) {
    if (r.kind != MIRExprKind::Place || !v) return;
    if (v->val_kind() == ValueKind::AccessChain) {
        auto* c = v->as_access_chain_unsafe();
        if (c->values.size() == 1) v = c->values[0];
    }
    if (v->val_kind() != ValueKind::FunctionCall) return;
    register_stmt_temp(static_cast<PlaceId>(r.id), v->getType());
}

void MIRLowerer::destroy_call_temp(Value* v, MIRExprResult r) {
    if (r.kind != MIRExprKind::Place || !v) return;
    if (v->val_kind() == ValueKind::AccessChain) {
        auto* c = v->as_access_chain_unsafe();
        if (c->values.size() == 1) v = c->values[0];
    }
    if (v->val_kind() != ValueKind::FunctionCall) return;
    register_stmt_temp(static_cast<PlaceId>(r.id), v->getType());
}

void MIRLowerer::register_stmt_temp(PlaceId place, BaseType* type) {
    if (place == MIR_INVALID_ID || !type) return;
    if (destructor_symbol(type) == MIR_INVALID_ID) return;
    for (auto& t : stmt_temps_) {
        if (t.first == place) return;
    }
    stmt_temps_.emplace_back(place, type);
}

void MIRLowerer::cancel_stmt_temp(PlaceId place) {
    for (size_t i = 0; i < stmt_temps_.size(); ++i) {
        if (stmt_temps_[i].first == place) {
            stmt_temps_.erase(stmt_temps_.begin() + static_cast<long>(i));
            return;
        }
    }
}

void MIRLowerer::flush_stmt_temps(size_t mark) {
    for (size_t i = mark; i < stmt_temps_.size(); ++i) {
        const SymbolId dtor = destructor_symbol(stmt_temps_[i].second);
        if (dtor == MIR_INVALID_ID) continue;
        builder_->destroy(stmt_temps_[i].first, dtor);
    }
    stmt_temps_.resize(mark);
}

bool MIRLowerer::apply_struct_defaults(PlaceId temp, ExtendableMembersContainerNode* def,
                                       const std::set<std::string>& explicit_fields,
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
        if (explicit_fields.count(std::string(bn.data(), bn.size()))) continue;
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
        const std::string nm(var->name.data(), var->name.size());
        if (explicit_fields.count(nm)) continue;
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, nm.data(), static_cast<uint32_t>(nm.size()));
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
                                                   const std::set<std::string>& explicit_fields) {
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
        const std::string nm(var->name.data(), var->name.size());
        if (explicit_fields.count(nm)) continue;
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
bool MIRLowerer::constructor_writes_whole_value(FunctionDeclaration* ctor) {
    if (!ctor || !ctor->body.has_value()) return false;
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
        const std::string nm(var->name.data(), var->name.size());
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, nm.data(), static_cast<uint32_t>(nm.size()));
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

bool MIRLowerer::store_anon_aggregate(ValueId addr, StructValue* sv, std::string& error) {
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
        if (init.value && init.value->val_kind() == ValueKind::StructValue) {
            ASTNode* mn = mt ? mt->get_direct_linked_canonical_node() : nullptr;
            if (mn && (mn->kind() == ASTNodeKind::UnnamedUnion ||
                       mn->kind() == ASTNodeKind::UnnamedStruct)) {
                const TypeId ft = types_.map(mt);
                const ValueId faddr = builder_->field_addr_ptr(addr, fc, ft);
                if (!store_anon_aggregate(faddr, init.value->as_struct_value_unsafe(), error)) {
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

void MIRLowerer::mark_moved(PlaceId place) {
    for (auto& d : destructibles_) {
        if (d.place == place) {
            builder_->set_drop(d.flag, false);
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

void MIRLowerer::emit_drops_from(size_t mark) {
    for (size_t i = destructibles_.size(); i-- > mark;) {
        const ValueId fv = builder_->load(destructibles_[i].flag, types_.bool_type());
        builder_->drop(destructibles_[i].place, destructibles_[i].dtor, fv);
    }
}

MIRExprResult MIRLowerer::lower_arg_converted(Value* arg, BaseType* param_type,
                                              std::string& error) {
    BaseType* conv_type = param_type;
    if (param_type) {
        if (param_type->kind() == BaseTypeKind::Reference) {
            conv_type = param_type->as_reference_type()->type;
        } else if (param_type->kind() == BaseTypeKind::Pointer) {
            conv_type = param_type->as_pointer_type()->type;
        }
    }
    // an untyped constructor call for the parameter's own type
    // (`std::string_view("...")`): the callee identifier may not be linked, so
    // match its name against the parameter type and construct directly.
    if (conv_type && arg && arg->val_kind() == ValueKind::FunctionCall) {
        auto* acall = arg->as_func_call_unsafe();
        std::string cname;
        bool have = false;
        if (acall->parent_val) {
            if (acall->parent_val->val_kind() == ValueKind::Identifier) {
                auto* id = acall->parent_val->as_identifier_unsafe();
                cname.assign(id->value.data(), id->value.size());
                have = true;
            } else if (acall->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* ch = acall->parent_val->as_access_chain_unsafe();
                std::string e;
                if (!ch->values.empty() && member_name(ch->values.back(), cname, e)) have = true;
            }
        }
        ASTNode* ln = conv_type->get_direct_linked_canonical_node();
        if (!ln) ln = conv_type->get_direct_linked_node();
        if ((!ln || ln->kind() != ASTNodeKind::StructDecl) &&
            conv_type->kind() == BaseTypeKind::Generic) {
            if (auto* g = conv_type->as_generic_type()) {
                if (g->referenced) {
                    ln = g->referenced->get_direct_linked_canonical_node();
                    if (!ln) ln = g->referenced->get_direct_linked_node();
                }
            }
        }
        if (have && ln && ln->kind() == ASTNodeKind::StructDecl) {
            auto* sd = ln->as_struct_def_unsafe();
            if (sd->name_view() ==
                chem::string_view(cname.data(), static_cast<unsigned>(cname.size()))) {
                FunctionDeclaration* ctor = sd->constructor_func(acall->values);
                if (!ctor && acall->values.empty()) ctor = conv_type->get_def_constructor();
                const TypeId mt = types_.map(conv_type);
                const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
                std::vector<MIROperand> cargs;
                const bool ctor_self = ctor && ctor->get_self_param() != nullptr;
                if (!lower_call_args_for(ctor, ctor_self, acall->values, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && !append_default_args(ctor, acall->values.size(), ctor_self, cargs,
                                                 error)) {
                    return MIRExprResult::error();
                }
                if (ctor && ctor->is_comptime() && comptime_eval_) {
                    Value* ev = comptime_eval_(acall, ctor);
                    if (ev) return lower_expr(ev, error);
                }
                if (ctor) {
                    const SymbolId sym = intern_function(ctor);
                    builder_->zero_init(tmp);
                    builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                }
                return MIRExprResult::place(tmp, mt);
            }
        }
    }
    if (conv_type) {
        // an untyped struct literal (`f({ a: 1, b: 2 })`) takes its type from the
        // expected parameter/return type
        if (arg && arg->val_kind() == ValueKind::StructValue) {
            BaseType* at = arg->getType();
            ASTNode* an = at ? at->get_direct_linked_canonical_node() : nullptr;
            if (!an) {
                BaseType* saved = expected_struct_type_;
                expected_struct_type_ = conv_type;
                MIRExprResult r = lower_expr(arg, error);
                expected_struct_type_ = saved;
                return r;
            }
        }
        // a lambda converted to a `std::function` is wrapped by the instance
        // type's comptime `make` constructor (fn ptr + closure data)
        CapturingFunctionType* cap = nullptr;
        if (conv_type) cap = conv_type->get_cap_func_type();
        if (!cap && arg && arg->val_kind() == ValueKind::LambdaFunc && arg->getType()) {
            cap = arg->getType()->get_cap_func_type();
        }
        // `std::function<T>` may resolve to the instance struct directly (a
        // `%capture` typealias instantiation), so also accept a struct target
        BaseType* cap_inst = cap ? const_cast<BaseType*>(cap->instance_type.getType()) : nullptr;
        if (!cap_inst && conv_type) {
            ASTNode* cln = conv_type->get_direct_linked_canonical_node();
            if (cln && cln->kind() == ASTNodeKind::StructDecl) {
                cap_inst = cln->as_struct_def_unsafe()->known_type();
            }
        }
        if (cap_inst && arg && arg->val_kind() == ValueKind::LambdaFunc) {
            BaseType* inst = cap_inst;
            FunctionDeclaration* imp = nullptr;
            ASTNode* in = inst ? inst->get_direct_linked_canonical_node() : nullptr;
            if (in && in->kind() == ASTNodeKind::StructDecl) {
                // the closure wrapper is the instance type's `make` constructor
                ASTNode* mk = in->as_struct_def_unsafe()->child("make");
                if (mk && mk->kind() == ASTNodeKind::FunctionDecl) {
                    imp = mk->as_function_unsafe();
                }
            }
            if (!imp && inst) imp = inst->implicit_constructor_for(arg);
            if (imp) {
                if (imp->is_comptime() && comptime_ctor_eval_) {
                    Value* ev = comptime_ctor_eval_(imp, arg);
                    if (ev) return lower_expr(ev, error);
                }
                const TypeId mt = types_.map(inst);
                const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
                std::vector<MIROperand> cargs;
                std::vector<Value*> one{arg};
                if (!lower_call_args(one, cargs, error)) return MIRExprResult::error();
                const SymbolId sym = intern_function(imp);
                builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                return MIRExprResult::place(tmp, mt);
            }
        }
        FunctionDeclaration* imp = nullptr;
        ASTNode* ln = conv_type->get_direct_linked_canonical_node();
        if (ln && ln->kind() == ASTNodeKind::StructDecl) {
            imp = ln->as_struct_def_unsafe()->implicit_constructor_func(arg);
        }
        if (!imp) imp = conv_type->implicit_constructor_for(arg);
        if (imp) {
            if (imp->is_comptime() && comptime_ctor_eval_) {
                Value* ev = comptime_ctor_eval_(imp, arg);
                if (ev) return lower_expr(ev, error);
            }
            const TypeId mt = types_.map(conv_type);
            const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
            std::vector<MIROperand> cargs;
            std::vector<Value*> one{arg};
            if (!lower_call_args(one, cargs, error)) return MIRExprResult::error();
            const SymbolId sym = intern_function(imp);
            builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
            return MIRExprResult::place(tmp, mt);
        }
    }
    return lower_expr(arg, error);
}

bool MIRLowerer::push_call_arg(MIRExprResult r, std::vector<MIROperand>& args,
                               std::string& error) {
    if (r.kind == MIRExprKind::Place) {
        const PlaceId pid = static_cast<PlaceId>(r.id);
        const TypeId pt = builder_->function().places[pid].type;
        const MIRTypeRecord& prec = module_.types.get(pt);
        if (prec.kind == MIRTypeKind::Pointer || prec.kind == MIRTypeKind::Reference) {
            args.push_back(MIROperand::value(builder_->load(pid, pt), pt));
        } else if (prec.kind == MIRTypeKind::Array) {
            // arrays decay to a pointer to their first element
            const TypeId eptr = types_.pointer_type(prec.element, false);
            args.push_back(MIROperand::value(builder_->address_of(pid, eptr), eptr));
        } else if (abi::is_aggregate(module_, pt)) {
            const TypeId ptrt = types_.pointer_type(pt, false);
            args.push_back(MIROperand::value(builder_->address_of(pid, ptrt), ptrt));
        } else {
            // a typedef struct (e.g. a dynamic fat pointer) is passed by value
            args.push_back(MIROperand::value(builder_->load(pid, pt), pt));
        }
    } else if (r.kind == MIRExprKind::Address) {
        args.push_back(MIROperand::value(static_cast<ValueId>(r.id), r.type));
    } else if (r.kind == MIRExprKind::Void) {
        error = "void expression used as a call argument";
        return false;
    } else {
        args.push_back(MIROperand::value(static_cast<ValueId>(r.id), r.type));
    }
    return true;
}

bool MIRLowerer::append_default_args(FunctionDeclaration* fd, size_t provided,
                                     bool self_included, std::vector<MIROperand>& args,
                                     std::string& error) {
    if (!fd) return true;
    auto& params = fd->params;
    const size_t offset = self_included ? 1 : 0;
    for (size_t i = provided + offset; i < params.size(); ++i) {
        if (fd->isVariadic() && i + 1 == params.size()) break; // variadic tail
        Value* dv = params[i]->defValue;
        if (!dv) {
            error = "missing required argument '" +
                    std::string(params[i]->name.data(), params[i]->name.size()) + "'";
            return false;
        }
        MIRExprResult r = lower_arg_converted(dv, params[i]->type, error);
        if (!r.ok()) return false;
        if (!push_call_arg(r, args, error)) return false;
    }
    return true;
}

MIRExprResult MIRLowerer::lower_member_chain(const std::vector<Value*>& values, size_t count,
                                             std::string& error) {
    if (count == 0) {
        error = "empty member chain";
        return MIRExprResult::error();
    }
    MIRExprResult first = lower_expr(values[0], error);
    if (!first.ok()) return first;
    if (count == 1) return first;
    // a struct-returning call used as a member/index base owns a temporary. A
    // scalar field read destroys it immediately (statement-local); an aggregate
    // field address escapes, so it is kept alive until the enclosing scope ends.
    const bool base_is_call = (values[0]->val_kind() == ValueKind::FunctionCall);
    PlaceId cur_place = MIR_INVALID_ID;
    ValueId cur_ptr = MIR_NULL;
    bool have_ptr = false;
    if (first.kind == MIRExprKind::Place) {
        cur_place = static_cast<PlaceId>(first.id);
    } else if (first.kind == MIRExprKind::Value || first.kind == MIRExprKind::Address) {
        cur_ptr = static_cast<ValueId>(first.id);
        have_ptr = true;
    } else {
        error = "member access base is not a place or pointer";
        return MIRExprResult::error();
    }
    for (size_t i = 1; i < count; ++i) {
        std::string fname;
        if (!member_field_path(values[i - 1], values[i], fname, error)) return MIRExprResult::error();
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
        const TypeId ft = types_.map(values[i]->getType());
        const bool last = (i + 1 == count);
        if (!have_ptr) {
            if (last) {
                if (abi::is_aggregate(module_, ft)) {
                    if (base_is_call) maybe_register_call_temp(values[0], first);
                    return MIRExprResult::address(builder_->field_addr(cur_place, fc, ft), ft);
                }
                const ValueId lv = builder_->field_load(cur_place, fc, ft);
                if (base_is_call) destroy_call_temp(values[0], first);
                return MIRExprResult::value(lv, ft);
            }
            const ValueId faddr = builder_->field_addr(cur_place, fc, ft);
            const MIRTypeRecord& ftr = module_.types.get(ft);
            cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                       ftr.kind == MIRTypeKind::Reference)
                          ? builder_->load_indirect(faddr, ft)
                          : faddr;
            have_ptr = true;
        } else {
            if (last) {
                if (abi::is_aggregate(module_, ft)) {
                    if (base_is_call) maybe_register_call_temp(values[0], first);
                    return MIRExprResult::address(builder_->field_addr_ptr(cur_ptr, fc, ft), ft);
                }
                const ValueId lv = builder_->field_load_ptr(cur_ptr, fc, ft);
                if (base_is_call) destroy_call_temp(values[0], first);
                return MIRExprResult::value(lv, ft);
            }
            const ValueId faddr = builder_->field_addr_ptr(cur_ptr, fc, ft);
            const MIRTypeRecord& ftr = module_.types.get(ft);
            cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                       ftr.kind == MIRTypeKind::Reference)
                          ? builder_->load_indirect(faddr, ft)
                          : faddr;
        }
    }
    error = "empty member chain";
    return MIRExprResult::error();
}

MIRExprResult MIRLowerer::lower_variant_payload(VariantCaseVariable* var, bool as_address,
                                                std::string& error) {
    if (!var || !var->member_param) {
        error = "invalid variant case variable";
        return MIRExprResult::error();
    }
    SwitchStatement* sw = var->parent();
    VariantMember* vm = var->member_param->parent();
    if (!sw || !sw->expression || !vm) {
        error = "variant case variable has no switch expression";
        return MIRExprResult::error();
    }
    MIRExprResult base = lower_expr(sw->expression, error);
    if (!base.ok()) return base;
    const std::string mname(vm->name.data(), vm->name.size());
    const std::string pname(var->member_param->name.data(), var->member_param->name.size());
    const std::string fname = mname + "." + pname;
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
    const TypeId ft = types_.map(var->known_type());
    if (base.kind == MIRExprKind::Place) {
        const PlaceId bp = static_cast<PlaceId>(base.id);
        if (as_address || abi::is_aggregate(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr(bp, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load(bp, fc, ft), ft);
    }
    if (base.kind == MIRExprKind::Address || base.kind == MIRExprKind::Value) {
        const ValueId bv = static_cast<ValueId>(base.id);
        if (as_address || abi::is_aggregate(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr_ptr(bv, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load_ptr(bv, fc, ft), ft);
    }
    error = "variant case expression is not a variant lvalue";
    return MIRExprResult::error();
}

MIRExprResult MIRLowerer::lower_pattern_base(PatternMatchExpr* pm, std::string& error) {
    auto it = pattern_bases_.find(pm);
    if (it != pattern_bases_.end()) return it->second;
    MIRExprResult base = lower_expr(pm->expression, error);
    if (!base.ok()) return base;
    pattern_bases_[pm] = base;
    return base;
}

MIRExprResult MIRLowerer::lower_pattern_payload(PatternMatchExpr* pm, VariantMemberParam* param,
                                                bool as_address, std::string& error) {
    if (!pm || !pm->member || !param) {
        error = "invalid pattern match payload";
        return MIRExprResult::error();
    }
    MIRExprResult base = lower_pattern_base(pm, error);
    if (!base.ok()) return base;
    const chem::string_view mname = pm->member->name;
    const chem::string_view pname = param->name;
    std::string fname(mname.data(), mname.size());
    fname.push_back('.');
    fname.append(pname.data(), pname.size());
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
    const TypeId ft = types_.map(param->type);
    if (base.kind == MIRExprKind::Place) {
        const PlaceId bp = static_cast<PlaceId>(base.id);
        if (as_address || abi::is_aggregate(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr(bp, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load(bp, fc, ft), ft);
    }
    if (base.kind == MIRExprKind::Address || base.kind == MIRExprKind::Value) {
        const ValueId bv = static_cast<ValueId>(base.id);
        if (as_address || abi::is_aggregate(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr_ptr(bv, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load_ptr(bv, fc, ft), ft);
    }
    error = "pattern match expression is not a variant lvalue";
    return MIRExprResult::error();
}

MIRExprResult MIRLowerer::lower_pattern_cond(PatternMatchExpr* pm, std::string& error) {
    if (!pm || !pm->member) {
        error = "invalid pattern match condition";
        return MIRExprResult::error();
    }
    MIRExprResult base = lower_pattern_base(pm, error);
    if (!base.ok()) return base;
    const std::string tag_name("__chx__vt_621827");
    const ConstantId tag_fc = module_.constants.add_string(
        MIR_INVALID_ID, tag_name.data(), static_cast<uint32_t>(tag_name.size()));
    const TypeId it = types_.u32_type();
    ValueId tag = MIR_NULL;
    if (base.kind == MIRExprKind::Place) {
        tag = builder_->field_load(static_cast<PlaceId>(base.id), tag_fc, it);
    } else if (base.kind == MIRExprKind::Address || base.kind == MIRExprKind::Value) {
        tag = builder_->field_load_ptr(static_cast<ValueId>(base.id), tag_fc, it);
    } else {
        error = "pattern match expression is not a variant lvalue";
        return MIRExprResult::error();
    }
    VariantDefinition* vd = pm->member->parent();
    const long idx = vd ? vd->direct_child_index(pm->member->name) : 0;
    const ValueId c = builder_->const_int(
        it, module_.constants.add_int(it, static_cast<uint64_t>(idx < 0 ? 0 : idx)));
    const TypeId bool_t = types_.bool_type();
    const ConstantId pred = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));
    return MIRExprResult::value(builder_->compare(tag, c, pred, bool_t), bool_t);
}

MIRExprResult MIRLowerer::lower_pattern_match_value(PatternMatchExpr* pm, std::string& error) {
    const PatternElseExprKind k = pm->elseExpression.kind;
    MIRExprResult cond = lower_pattern_cond(pm, error);
    if (!cond.ok()) return cond;

    // `if(var Some(x) = v)`: a plain condition; the payload is read through the
    // cached base when `x` is used in the body.
    if (k == PatternElseExprKind::None) return cond;

    auto store_into = [&](PlaceId dest, TypeId dest_t, MIRExprResult r) -> bool {
        if (r.kind == MIRExprKind::Place) {
            builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), dest_t));
        } else if (r.kind == MIRExprKind::Address) {
            builder_->store(dest, builder_->load_indirect(static_cast<ValueId>(r.id), dest_t));
        } else if (r.kind == MIRExprKind::Value) {
            builder_->store(dest, static_cast<ValueId>(r.id));
        } else {
            error = "pattern match operand does not produce a value";
            return false;
        }
        return true;
    };

    if (k == PatternElseExprKind::DefValue) {
        if (pm->param_names.empty() || !pm->param_names[0] || !pm->param_names[0]->member_param) {
            error = "invalid pattern match payload";
            return MIRExprResult::error();
        }
        const TypeId pt = types_.map(pm->param_names[0]->member_param->type);
        const PlaceId res = builder_->alloca(pt, MIRStorageClass::Temporary);
        const BlockId then_b = builder_->create_block();
        const BlockId else_b = builder_->create_block();
        const BlockId merge = builder_->create_block();
        builder_->cond_br(cond.id, then_b, else_b);
        builder_->set_block(then_b);
        MIRExprResult payload =
            lower_pattern_payload(pm, pm->param_names[0]->member_param, false, error);
        if (!payload.ok()) return payload;
        if (!store_into(res, pt, payload)) return MIRExprResult::error();
        if (!builder_->current_block_terminated()) builder_->br(merge);
        builder_->set_block(else_b);
        MIRExprResult def = lower_expr(pm->elseExpression.value, error);
        if (!def.ok()) return def;
        if (!store_into(res, pt, def)) return MIRExprResult::error();
        if (!builder_->current_block_terminated()) builder_->br(merge);
        builder_->set_block(merge);
        pattern_results_[pm] = MIRExprResult::place(res, pt);
        if (abi::is_aggregate(module_, pt)) return MIRExprResult::place(res, pt);
        return MIRExprResult::value(builder_->load(res, pt), pt);
    }

    // Return / Break / Continue / Unreachable: guard on the tag, then the payload
    const BlockId then_b = builder_->create_block();
    const BlockId else_b = builder_->create_block();
    builder_->cond_br(cond.id, then_b, else_b);
    builder_->set_block(else_b);
    switch (k) {
        case PatternElseExprKind::Return: {
            ReturnStatement rs(pm->elseExpression.value, nullptr, pm->encoded_location());
            if (!lower_stmt(&rs, error)) return MIRExprResult::error();
            break;
        }
        case PatternElseExprKind::Break: {
            if (break_targets_.empty()) {
                error = "pattern break outside a loop";
                return MIRExprResult::error();
            }
            if (!loop_destructible_marks_.empty()) {
                emit_drops_from(loop_destructible_marks_.back());
            }
            builder_->br(break_targets_.back());
            break;
        }
        case PatternElseExprKind::Continue: {
            if (continue_targets_.empty()) {
                error = "pattern continue outside a loop";
                return MIRExprResult::error();
            }
            if (!loop_destructible_marks_.empty()) {
                emit_drops_from(loop_destructible_marks_.back());
            }
            builder_->br(continue_targets_.back());
            break;
        }
        case PatternElseExprKind::Unreachable:
            builder_->unreachable();
            break;
        default:
            builder_->br(then_b);
            break;
    }
    builder_->set_block(then_b);
    VariantMemberParam* first =
        (!pm->param_names.empty() && pm->param_names[0]) ? pm->param_names[0]->member_param
                                                         : nullptr;
    return lower_pattern_payload(pm, first, false, error);
}

bool MIRLowerer::lower_pattern_match_node(PatternMatchExprNode* node, std::string& error) {
    MIRExprResult r = lower_pattern_match_value(&node->value, error);
    return r.ok();
}

MIRExprResult MIRLowerer::lower_pattern_identifier(PatternMatchIdentifier* id,
                                                   std::string& error) {
    PatternMatchExpr* pm = id->matchExpr;
    if (!pm) {
        error = "pattern identifier has no match expression";
        return MIRExprResult::error();
    }
    // a DefValue result temp holds only the first destructured parameter; the
    // other parameters read their own payload field
    VariantMemberParam* first =
        (!pm->param_names.empty() && pm->param_names[0]) ? pm->param_names[0]->member_param
                                                         : nullptr;
    if (first && id->member_param == first) {
        auto it = pattern_results_.find(pm);
        if (it != pattern_results_.end()) {
            MIRExprResult r = it->second;
            if (r.kind == MIRExprKind::Place) {
                const TypeId t = r.type;
                if (abi::is_aggregate(module_, t)) return r;
                return MIRExprResult::value(builder_->load(static_cast<PlaceId>(r.id), t), t);
            }
            return r;
        }
    }
    return lower_pattern_payload(pm, id->member_param, false, error);
}

MIRExprResult MIRLowerer::lower_receiver(const std::vector<Value*>& values, size_t count,
                                         std::string& error) {
    if (count == 1 && values[0]) {
        Value* v = values[0];
        if (v->val_kind() == ValueKind::AccessChain) {
            auto* c = v->as_access_chain_unsafe();
            if (c->values.size() == 1) v = c->values[0];
        }
        const ValueKind k = v->val_kind();
        if (k == ValueKind::DereferenceValue) {
            // the pointer itself is the receiver address
            MIRExprResult p = lower_expr(v->as_dereference_value_unsafe()->getValue(), error);
            if (!p.ok()) return p;
            return MIRExprResult::address(static_cast<ValueId>(p.id), p.type);
        }
        if (k == ValueKind::Identifier || k == ValueKind::AccessChain ||
            k == ValueKind::IndexOperator) {
            // an unqualified field receiver inside a method resolves through
            // `self` (handled by `lower_expr` as an implicit member access)
            if (k == ValueKind::Identifier) {
                auto* rid = v->as_identifier_unsafe();
                if (is_implicit_field_node(rid->linked) && current_self_param_) {
                    return lower_expr(v, error);
                }
            }
            // prefer the place (needed for `&self` scalar receivers)
            std::string perr;
            const PlaceId p = resolve_place_or_lower(v, perr);
            if (p != MIR_INVALID_ID) {
                const TypeId pt = builder_->function().places[p].type;
                return MIRExprResult::place(p, pt);
            }
            MIRExprResult a = lower_address_of(v, error);
            if (a.ok()) return a;
        }
        if (k == ValueKind::FunctionCall) {
            // a struct-returning call as a method receiver: keep its temporary
            // alive until the end of the statement (destroyed after the chain)
            MIRExprResult r = lower_expr(v, error);
            if (!r.ok()) return r;
            maybe_register_call_temp(v, r);
            return r;
        }
    }
    return lower_member_chain(values, count, error);
}

MIRExprResult MIRLowerer::lower_method_call(MIRExprResult recv, BaseType* recv_type,
                                            FunctionCall* call, std::string& error) {
    ASTNode* lk = call->parent_val ? call->parent_val->linked_node() : nullptr;
    FunctionDeclaration* fd = lk ? lk->as_function() : nullptr;
    if (!fd && call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
        auto* ch = call->parent_val->as_access_chain_unsafe();
        if (!ch->values.empty()) {
            std::string mname, merr;
            if (member_name(ch->values.back(), mname, merr)) {
                fd = resolve_method_fallback(recv_type, mname);
            }
        }
    }
    if (!fd) {
        error = "method call to unresolved function";
        return MIRExprResult::error();
    }
    if (!recv.ok()) return recv;

    // a comptime method must be evaluated before its arguments are lowered (the
    // arguments may be comptime-only values such as expressive strings)
    if (fd->is_comptime() && comptime_eval_) {
        Value* evaluated = comptime_eval_(call, fd);
        if (!evaluated) {
            // a void comptime call whose evaluation produced no value (e.g. an
            // inline-asm fence) is a no-op
            if (is_void_type(module_, types_.map(fd->returnType))) {
                return MIRExprResult::void_result();
            }
            error = "comptime call to '" + fd->name_str() + "' could not be evaluated";
            return MIRExprResult::error();
        }
        return lower_expr(evaluated, error);
    }

    // dynamic (interface) dispatch: `((vt_t*) fat.second)->method(fat.first, args...)`
    if (recv_type && recv_type->canonical() &&
        recv_type->canonical()->kind() == BaseTypeKind::Dynamic) {
        auto* dt = recv_type->canonical()->as_dynamic_type_unsafe();
        BaseType* iface_type = dt ? dt->referenced : nullptr;
        ASTNode* iface_node =
            iface_type ? iface_type->get_direct_linked_canonical_node() : nullptr;
        if (!iface_node) {
            error = "dynamic receiver interface could not be resolved";
            return MIRExprResult::error();
        }
        std::string iface_name;
        if (mangler_) iface_name = mangler_(iface_node);
        if (iface_name.empty()) {
            const chem::string_view nv = iface_node->as_interface_def_unsafe()->name_view();
            iface_name.assign(nv.data(), nv.size());
        }
        const std::string vt_name = "__chx_" + iface_name + "_vt_t";
        const ConstantId vt_c = module_.constants.add_string(
            MIR_INVALID_ID, vt_name.data(), static_cast<uint32_t>(vt_name.size()));
        const chem::string_view mv = fd->name_view();
        const std::string m_name(mv.data(), mv.size());
        const ConstantId m_c = module_.constants.add_string(
            MIR_INVALID_ID, m_name.data(), static_cast<uint32_t>(m_name.size()));
        ValueId fat = MIR_NULL;
        TypeId fat_t = recv.type;
        if (recv.kind == MIRExprKind::Place) {
            fat_t = builder_->function().places[recv.id].type;
            fat = builder_->load(recv.id, fat_t);
        } else if (recv.kind == MIRExprKind::Address) {
            // an lvalue address: the operand is a pointer to the fat pointer
            fat_t = types_.pointer_type(recv.type, true);
            fat = static_cast<ValueId>(recv.id);
        } else {
            fat = static_cast<ValueId>(recv.id);
        }
        const size_t cmark = call_temps_.size();
        std::vector<MIROperand> dargs;
        if (!lower_call_args_for(fd, true, call->values, dargs, error)) {
            return MIRExprResult::error();
        }
        if (!append_default_args(fd, call->values.size(), true, dargs, error)) {
            return MIRExprResult::error();
        }
        const TypeId dtype = types_.map(call->getType());
        if (abi::is_aggregate(module_, dtype)) {
            error = "dynamic method returning an aggregate is not yet supported";
            return MIRExprResult::error();
        }
        const ValueId dv = builder_->dynamic_call(
            fat, fat_t, vt_c, m_c, dargs.data(), static_cast<uint32_t>(dargs.size()), dtype);
        destroy_call_temps_from(cmark, error);
        if (is_void_type(module_, dtype)) return MIRExprResult::void_result();
        return MIRExprResult::value(dv, dtype);
    }

    std::vector<MIROperand> args;
    if (!append_receiver_arg(recv, recv_type, fd, args, error)) {
        return MIRExprResult::error();
    }
    const size_t cmark = call_temps_.size();
    if (!lower_call_args_for(fd, true, call->values, args, error)) {
        return MIRExprResult::error();
    }
    if (!append_default_args(fd, call->values.size(), true, args, error)) {
        return MIRExprResult::error();
    }

    const SymbolId sym = intern_method_function(fd, recv_type);
    const TypeId type = types_.map(call->getType());
    return emit_resolved_call(sym, type, args, cmark, error);
}

bool MIRLowerer::append_receiver_arg(MIRExprResult recv, BaseType* recv_type,
                                     FunctionDeclaration* fd, std::vector<MIROperand>& args,
                                     std::string& error) {
    // the self parameter (methods and extension functions)
    FunctionParam* self = fd->get_self_param();
    if (!self && fd->isExtensionFn() && !fd->params.empty()) self = fd->params[0];
    BaseType* self_t = self ? self->type : nullptr;
    const bool by_value =
        !self_t || (self_t->kind() != BaseTypeKind::Reference &&
                    self_t->kind() != BaseTypeKind::Pointer);
    BaseType* x_type = nullptr;
    if (self_t) {
        if (self_t->kind() == BaseTypeKind::Reference) {
            x_type = self_t->as_reference_type()->type;
        } else if (self_t->kind() == BaseTypeKind::Pointer) {
            x_type = self_t->as_pointer_type()->type;
        }
    }

    if (by_value) {
        if (recv.kind == MIRExprKind::Place) {
            const TypeId rt = builder_->function().places[recv.id].type;
            if (abi::is_aggregate(module_, rt)) {
                // an aggregate `self` is passed as a hidden pointer
                const TypeId ptrt = types_.pointer_type(rt, true);
                args.push_back(MIROperand::value(builder_->address_of(recv.id, ptrt), ptrt));
            } else {
                args.push_back(MIROperand::value(builder_->load(recv.id, rt), rt));
            }
        } else {
            args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), recv.type));
        }
    } else if (recv.kind == MIRExprKind::Place) {
        const TypeId rt = builder_->function().places[recv.id].type;
        const MIRTypeRecord& rr = module_.types.get(rt);
        const bool is_ptr = (rr.kind == MIRTypeKind::Pointer ||
                             rr.kind == MIRTypeKind::Reference);
        // the receiver is already a pointer/reference whose pointee is the
        // method's target type (or a Chemical struct derived from it): pass the
        // pointer value so the C compiler performs the implicit derived->base
        // upcast. Otherwise (e.g. `&int` receiver for a `&&int` method) we must
        // take the address of the place instead.
        bool pass_pointer = false;
        if (is_ptr && x_type) {
            const TypeId xt = types_.map(x_type);
            if (rr.element == xt) {
                pass_pointer = true;
            } else if (recv_type) {
                BaseType* rc = recv_type->canonical();
                BaseType* recv_elem = nullptr;
                if (rc && rc->kind() == BaseTypeKind::Pointer) {
                    recv_elem = rc->as_pointer_type()->type;
                } else if (rc && rc->kind() == BaseTypeKind::Reference) {
                    recv_elem = rc->as_reference_type()->type;
                }
                if (recv_elem) {
                    ASTNode* en = recv_elem->get_direct_linked_canonical_node();
                    ASTNode* xn = x_type->get_direct_linked_canonical_node();
                    if (en && en->kind() == ASTNodeKind::StructDecl &&
                        xn && xn->kind() == ASTNodeKind::StructDecl) {
                        pass_pointer = struct_derives_from(en->as_struct_def_unsafe(),
                                                           xn->known_type());
                    }
                }
            }
        }
        if (pass_pointer) {
            args.push_back(MIROperand::value(builder_->load(recv.id, rt), rt));
        } else {
            const TypeId ptrt = types_.pointer_type(rt, true);
            args.push_back(MIROperand::value(builder_->address_of(recv.id, ptrt), ptrt));
        }
    } else if (recv.kind == MIRExprKind::Address) {
        // an lvalue address (field / element): already a pointer to the receiver
        const TypeId ptrt = types_.pointer_type(recv.type, true);
        args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), ptrt));
    } else {
        // an rvalue: materialize a temporary and pass its address
        const MIRTypeRecord& rr = module_.types.get(recv.type);
        const bool is_ptr = (rr.kind == MIRTypeKind::Pointer ||
                             rr.kind == MIRTypeKind::Reference);
        if (is_ptr && x_type && rr.element == types_.map(x_type)) {
            args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), recv.type));
        } else if (rr.kind == MIRTypeKind::Opaque && self) {
            // untyped receiver node: trust the resolved method's receiver type
            const TypeId st = types_.map(self->type);
            args.push_back(MIROperand::value(static_cast<ValueId>(recv.id), st));
        } else {
            const TypeId rt = recv.type;
            const PlaceId tmp = builder_->alloca(rt, MIRStorageClass::Temporary);
            builder_->store(tmp, static_cast<ValueId>(recv.id));
            const TypeId ptrt = types_.pointer_type(rt, true);
            args.push_back(MIROperand::value(builder_->address_of(tmp, ptrt), ptrt));
        }
    }
    return true;
}

MIRExprResult MIRLowerer::emit_resolved_call(SymbolId sym, TypeId ret_type,
                                             std::vector<MIROperand>& args, size_t cmark,
                                             std::string& error) {
    if (is_void_type(module_, ret_type)) {
        builder_->call_scalar(sym, ret_type, args.data(), static_cast<uint32_t>(args.size()));
        destroy_call_temps_from(cmark, error);
        return MIRExprResult::void_result();
    }
    if (abi::needs_sret(module_, ret_type)) {
        const PlaceId res = builder_->alloca(ret_type, MIRStorageClass::Temporary);
        if (res == MIR_INVALID_ID) {
            error = "failed to allocate call result place";
            return MIRExprResult::error();
        }
        builder_->call_sret(sym, res, args.data(), static_cast<uint32_t>(args.size()));
        destroy_call_temps_from(cmark, error);
        return MIRExprResult::place(res, ret_type);
    }
    const ValueId v =
        builder_->call_scalar(sym, ret_type, args.data(), static_cast<uint32_t>(args.size()));
    destroy_call_temps_from(cmark, error);
    return MIRExprResult::value(v, ret_type);
}

MIRExprResult MIRLowerer::lower_operator_overload(Value* lhs_v, MIRExprResult lhs, Value* rhs_v,
                                                  MIRExprResult rhs, const std::string& method,
                                                  BaseType* ret_ast, std::string& error) {
    if (!lhs.ok()) return lhs;
    if (!rhs.ok()) return rhs;
    BaseType* lhs_t = lhs_v ? lhs_v->getType() : nullptr;
    BaseType* rhs_t = rhs_v ? rhs_v->getType() : nullptr;
    FunctionDeclaration* fd = resolve_method_fallback(lhs_t, method);
    if (!fd && rhs_t) fd = resolve_method_fallback(rhs_t, method);
    if (!fd) {
        error = "could not resolve operator method '" + method + "'";
        return MIRExprResult::error();
    }
    // synthesize a call node so the method-call machinery (receiver + args +
    // implicit conversions) can be reused
    VariableIdentifier id(chem::string_view(method.data(), method.size()), nullptr, ZERO_LOC);
    id.linked = fd;
    FunctionCall call(&id, ret_ast, ZERO_LOC);
    call.values.push_back(rhs_v);
    return lower_method_call(lhs, lhs_t, &call, error);
}

MIRExprResult MIRLowerer::lower_unary_operator_overload(Value* operand_v, MIRExprResult operand,
                                                        const char* method, BaseType* ret_ast,
                                                        std::string& error) {
    if (!operand.ok()) return operand;
    BaseType* op_t = operand_v ? operand_v->getType() : nullptr;
    FunctionDeclaration* fd = resolve_method_fallback(op_t, method);
    if (!fd) {
        error = "could not resolve operator method '" + std::string(method) + "'";
        return MIRExprResult::error();
    }
    VariableIdentifier id(chem::string_view(method, std::strlen(method)), nullptr, ZERO_LOC);
    id.linked = fd;
    FunctionCall call(&id, ret_ast, ZERO_LOC);
    return lower_method_call(operand, op_t, &call, error);
}

SymbolId MIRLowerer::intern_global(VarInitStatement* vi) {
    auto it = global_symbols_.find(vi);
    if (it != global_symbols_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Global;
    // Only the module whose files declare this global *defines* it. A global
    // reached from another module (symres links the identifier straight to the
    // declaring node) is a reference to that module's symbol: `External` here,
    // an LLVM declaration there. Emitting a definition in both objects is a
    // link error ("multiple definition of `submod_extern_globe_var'").
    const bool defined_here =
        !vi->is_extern() && owned_globals_.find(vi) != owned_globals_.end();
    rec.linkage = defined_here ? MIRLinkage::Internal : MIRLinkage::External;
    if (vi->getType()) {
        BaseType* gt = vi->getType();
        if (gt->kind() == BaseTypeKind::Array && !gt->as_array_type()->has_array_size()) {
            // an unsized array global is declared by the legacy as a C array and
            // decays to a pointer on use
            rec.type = types_.unsized_array_type(
                types_.map(gt->as_array_type()->known_child_type()));
        } else {
            rec.type = types_.map(gt);
        }
    }
    std::string name;
    if (mangler_) name = mangler_(vi);
    if (name.empty()) {
        const chem::string_view nv = vi->name_view();
        name.assign(nv.data(), nv.size());
    }
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    global_symbols_[vi] = s;
    return s;
}

// ── global initializer constant expressions (action A3) ─────────────────────
//
// The target-neutral initializer tree (MIRConstExpr.h). This is the single AST
// walk that used to be duplicated by the C emitter (`const_value_expr`) and the
// LLVM emitter (`GlobalConstantLowerer`); both now render this tree instead.

ConstExprId MIRLowerer::build_global_initializer(VarInitStatement* vi) {
    const SymbolId sym = intern_global(vi);
    ConstExprId init = MIR_INVALID_ID;
    if (vi->value) init = build_const_expr(vi->value);
    MIRGlobal g;
    g.symbol = sym;
    g.init = init;
    if (vi->is_thread_local()) g.flags |= MGD_THREAD_LOCAL;
    if (vi->is_volatile()) g.flags |= MGD_VOLATILE;
    module_.globals.push_back(g);
    return init;
}

ConstExprId MIRLowerer::build_const_expr(Value* value) {
    auto type_of = [&](Value* v) -> TypeId {
        return (v && v->getType()) ? types_.map(v->getType()) : MIR_INVALID_ID;
    };
    if (!value) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    switch (value->val_kind()) {
        case ValueKind::IntN: {
            auto* v = value->as_int_num_value_unsafe();
            const ConstExprId id = module_.const_exprs.add_int(type_of(value), v->get_num_value());
            if (v->is_unsigned()) module_.const_exprs.exprs[id].flags |= MCF_UNSIGNED;
            return id;
        }
        case ValueKind::Float:
            return module_.const_exprs.add_float(type_of(value), value->as_float_unsafe()->value);
        case ValueKind::Double:
            return module_.const_exprs.add_double(type_of(value),
                                                  value->as_double_unsafe()->value);
        case ValueKind::Bool:
            return module_.const_exprs.add_bool(type_of(value), value->as_bool_unsafe()->value);
        case ValueKind::String: {
            auto* sv = value->as_string_unsafe();
            return module_.const_exprs.add_string(type_of(value), sv->value.data(),
                                                  static_cast<uint32_t>(sv->value.size()));
        }
        case ValueKind::NullValue:
            return module_.const_exprs.add_null(type_of(value));
        case ValueKind::NegativeValue: {
            Value* inner = value->as_negative_value_unsafe()->getValue();
            return module_.const_exprs.add_negate(type_of(value), build_const_expr(inner));
        }
        case ValueKind::ComptimeValue: {
            Value* inner = static_cast<ComptimeValue*>(value)->getValue();
            return inner ? build_const_expr(inner)
                         : module_.const_exprs.add_zero(type_of(value));
        }
        case ValueKind::CastedValue:
            return build_const_expr(value->as_casted_value_unsafe()->value);
        case ValueKind::ArrayValue:
            return build_const_array(value->as_array_value_unsafe());
        case ValueKind::StructValue:
            return build_const_struct_value(value->as_struct_value_unsafe());
        case ValueKind::FunctionCall:
        case ValueKind::AccessChain:
            return build_const_call(value);
        default:
            return module_.const_exprs.add_zero(type_of(value));
    }
}

ConstExprId MIRLowerer::build_const_array(ArrayValue* av) {
    const TypeId t = (av && av->getType()) ? types_.map(av->getType()) : MIR_INVALID_ID;
    std::vector<MIRConstChild> kids;
    if (av) {
        kids.reserve(av->values.size());
        for (Value* v : av->values) {
            MIRConstChild c;
            c.expr = build_const_expr(v);
            kids.push_back(c);
        }
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Aggregate, t, kids);
}

ConstExprId MIRLowerer::build_const_struct_default(StructDefinition* sd) {
    if (!sd) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    const TypeId t = sd->known_type() ? types_.map(sd->known_type()) : MIR_INVALID_ID;
    std::vector<MIRConstChild> kids;
    auto add = [&](const chem::string_view& name, ConstExprId id) {
        MIRConstChild c;
        c.expr = id;
        c.name_offset = module_.const_exprs.intern_name(name.data(),
                                                       static_cast<uint32_t>(name.size()));
        c.name_length = static_cast<uint32_t>(name.size());
        kids.push_back(c);
    };
    for (auto& inh : sd->inherited) {
        StructDefinition* base = inh.type ? inh.type->get_direct_linked_struct() : nullptr;
        if (!base) continue;
        add(base->name_view(), build_const_struct_default(base));
    }
    for (BaseDefMember* var : sd->variables()) {
        if (!var) continue;
        Value* def = var->default_value();
        if (!def) continue;
        add(var->name, build_const_expr(def));
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Aggregate, t, kids, 0,
                                                 MCF_CAST_PREFIX);
}

ConstExprId MIRLowerer::build_const_struct_value(StructValue* sv) {
    const TypeId t = (sv && sv->getType()) ? types_.map(sv->getType()) : MIR_INVALID_ID;
    std::vector<MIRConstChild> kids;
    auto add = [&](const chem::string_view& name, ConstExprId id) {
        MIRConstChild c;
        c.expr = id;
        c.name_offset = module_.const_exprs.intern_name(name.data(),
                                                       static_cast<uint32_t>(name.size()));
        c.name_length = static_cast<uint32_t>(name.size());
        kids.push_back(c);
    };
    if (sv) {
        // explicit values, in source order (an `ordered_map`) — matching
        // `const_value_expr`
        for (auto& entry : sv->values) {
            add(entry.second.name, build_const_expr(entry.second.value));
        }
        StructDefinition* sd =
            sv->getType() ? sv->getType()->get_direct_linked_struct() : nullptr;
        if (sd) {
            for (auto& inh : sd->inherited) {
                StructDefinition* base =
                    inh.type ? inh.type->get_direct_linked_struct() : nullptr;
                if (!base) continue;
                if (sv->values.find(base->name_view()) != sv->values.end()) continue;
                add(base->name_view(), build_const_struct_default(base));
            }
            for (BaseDefMember* var : sd->variables()) {
                if (!var) continue;
                if (!sd->isDirectChild(var)) continue;
                if (sv->values.find(var->name) != sv->values.end()) continue;
                Value* def = var->default_value();
                if (!def) continue;
                add(var->name, build_const_expr(def));
            }
        }
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Aggregate, t, kids);
}

ConstExprId MIRLowerer::build_const_variant(VariantMember* vm, FunctionCall* call) {
    if (!vm) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    VariantDefinition* vd = vm->parent();
    const long idx = vd ? vd->direct_child_index(vm->name) : 0;
    const TypeId t = (call && call->getType()) ? types_.map(call->getType()) : MIR_INVALID_ID;
    const std::string case_name(vm->name.data(), vm->name.size());
    std::vector<MIRConstChild> kids;
    auto add = [&](const std::string& name, ConstExprId id) {
        MIRConstChild c;
        c.expr = id;
        c.name_offset = module_.const_exprs.intern_name(name.data(),
                                                       static_cast<uint32_t>(name.size()));
        c.name_length = static_cast<uint32_t>(name.size());
        kids.push_back(c);
    };
    size_t ai = 0;
    for (auto& entry : vm->values) {
        VariantMemberParam* param = entry.second;
        Value* arg = nullptr;
        if (call && ai < call->values.size()) {
            arg = call->values[ai];
            ai++;
        } else if (param) {
            arg = param->def_value;
        }
        if (!arg) continue;
        const std::string pname(entry.first.data(), entry.first.size());
        add(case_name + "." + pname, build_const_expr(arg));
    }
    if (vd) {
        for (auto& inh : vd->inherited) {
            StructDefinition* base =
                inh.type ? inh.type->get_direct_linked_struct() : nullptr;
            if (!base) continue;
            add(std::string(base->name_view().data(), base->name_view().size()),
                build_const_struct_default(base));
        }
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Variant, t, kids,
                                                 idx < 0 ? 0u : static_cast<uint32_t>(idx));
}

ConstExprId MIRLowerer::build_const_variant_bare(VariantMember* vm, BaseType* type) {
    if (!vm) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    VariantDefinition* vd = vm->parent();
    const long idx = vd ? vd->direct_child_index(vm->name) : 0;
    const TypeId t = type ? types_.map(type) : MIR_INVALID_ID;
    std::vector<MIRConstChild> kids;
    auto add = [&](const char* name, uint32_t len, ConstExprId id) {
        MIRConstChild c;
        c.expr = id;
        c.name_offset = module_.const_exprs.intern_name(name, len);
        c.name_length = len;
        kids.push_back(c);
    };
    if (vd) {
        for (auto& inh : vd->inherited) {
            StructDefinition* base =
                inh.type ? inh.type->get_direct_linked_struct() : nullptr;
            if (!base) continue;
            const chem::string_view nv = base->name_view();
            add(nv.data(), static_cast<uint32_t>(nv.size()), build_const_struct_default(base));
        }
    }
    return module_.const_exprs.add_with_children(MIRConstKind::Variant, t, kids,
                                                 idx < 0 ? 0u : static_cast<uint32_t>(idx));
}

ConstExprId MIRLowerer::build_const_call(Value* value) {
    auto type_of = [&](Value* v) -> TypeId {
        return (v && v->getType()) ? types_.map(v->getType()) : MIR_INVALID_ID;
    };
    if (!value) return module_.const_exprs.add_zero(MIR_INVALID_ID);
    if (value->val_kind() == ValueKind::AccessChain) {
        auto* ch = value->as_access_chain_unsafe();
        if (ch->values.empty()) return module_.const_exprs.add_zero(type_of(value));
        Value* last = ch->values.back();
        if (last->val_kind() == ValueKind::FunctionCall) return build_const_call(last);
        ASTNode* linked = last->linked_node();
        if (linked) {
            if (linked->kind() == ASTNodeKind::VariantMember) {
                return build_const_variant_bare(linked->as_variant_member_unsafe(),
                                                value->getType());
            }
            if (linked->kind() == ASTNodeKind::FunctionDecl) {
                return module_.const_exprs.add_symbol(
                    type_of(value), intern_function(linked->as_function_unsafe()));
            }
            if (linked->kind() == ASTNodeKind::VarInitStmt) {
                auto* vi = linked->as_var_init_unsafe();
                if (vi->value) return build_const_expr(vi->value);
            }
        }
        return module_.const_exprs.add_zero(type_of(value));
    }
    auto* fc = value->as_func_call_unsafe();
    ASTNode* vleaf = nullptr;
    if (fc->parent_val) {
        if (fc->parent_val->val_kind() == ValueKind::AccessChain) {
            auto* ch = fc->parent_val->as_access_chain_unsafe();
            if (!ch->values.empty()) vleaf = ch->values.back()->linked_node();
        } else {
            vleaf = fc->parent_val->linked_node();
        }
    }
    if (vleaf && vleaf->kind() == ASTNodeKind::VariantMember) {
        return build_const_variant(vleaf->as_variant_member_unsafe(), fc);
    }
    // a comptime function call folds to its constant result
    if (comptime_value_eval_) {
        Value* ev = comptime_value_eval_(fc);
        if (ev && ev != fc) return build_const_expr(ev);
    }
    return module_.const_exprs.add_zero(type_of(value));
}

SymbolId MIRLowerer::intern_named_global(const std::string& name) {
    auto it = named_globals_.find(name);
    if (it != named_globals_.end()) return it->second;
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Global;
    rec.linkage = MIRLinkage::External;
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    named_globals_[name] = s;
    return s;
}

SymbolId MIRLowerer::intern_function(FunctionDeclaration* decl) {
    auto it = func_symbols_.find(decl);
    if (it != func_symbols_.end()) return it->second;

    // The declaration ABI (sret for struct-like returns, aggregates/arrays
    // passed by pointer, the static-interface erased self, the variadic tail
    // marker dropped) is the shared `mir_function_signature`, so an extern
    // (bodyless) function's LLVM prototype is exactly the one the C declaration
    // pass writes and LLVM call sites expect.
    TypeId ftype = mir_function_signature(types_, module_, decl);

    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    // A `public` (or `protected`) function is visible across modules, so its
    // definition must have external linkage; the C backend uses the same rule.
    // Only interning the *call* symbol is not enough: for a function that is
    // never visited by `mir_translate_after_declaration`'s declaration pass
    // (e.g. one nested inside a top-level `if`'s taken scope) this is the only
    // place its symbol — and therefore its linkage — is decided.
    rec.linkage = (decl->is_extern() || decl->is_linkage_public()) ? MIRLinkage::External
                                                                  : MIRLinkage::Internal;
    rec.type = ftype;

    // NOTE: mangled name is filled by the serial symbol builder (PR3b) using
    // NameMangler; MIR must not retain FunctionDeclaration pointers.
    std::string name;
    if (mangler_) name = mangler_(decl);
    if (name.empty()) {
        const chem::string_view nv = decl->name_view();
        name.assign(nv.data(), nv.size());
    }
    SymbolId s = module_.symbols.add(rec, name.data(), static_cast<uint32_t>(name.size()),
                                     name.data(), static_cast<uint32_t>(name.size()));
    func_symbols_[decl] = s;
    return s;
}

/**
 * The `intrinsics::llvm::atomic_*` compiler declarations. They are the LLVM
 * backend's spelling of the atomic library's operations (the C backend takes
 * the `%runtime_value(__chx__*_dispatch(...))` path instead), so the MIR
 * emitter must model them as first-class atomic ops. Matched by name on a
 * compiler declaration: those nodes are only ever created by the compiler.
 */
enum class AtomicIntrinsic : uint8_t {
    None,
    Fence,
    SignalFence,
    Load,
    Store,
    CmpXchgWeak,
    CmpXchgStrong,
    Rmw,
};

static AtomicIntrinsic atomic_intrinsic_of(FunctionDeclaration* fd) {
    if (!fd || !fd->is_compiler_decl()) return AtomicIntrinsic::None;
    const chem::string_view n = fd->name_view();
    if (n == chem::string_view("atomic_fence")) return AtomicIntrinsic::Fence;
    if (n == chem::string_view("atomic_signal_fence")) return AtomicIntrinsic::SignalFence;
    if (n == chem::string_view("atomic_load")) return AtomicIntrinsic::Load;
    if (n == chem::string_view("atomic_store")) return AtomicIntrinsic::Store;
    if (n == chem::string_view("atomic_cmp_exch_weak")) return AtomicIntrinsic::CmpXchgWeak;
    if (n == chem::string_view("atomic_cmp_exch_strong")) return AtomicIntrinsic::CmpXchgStrong;
    if (n == chem::string_view("atomic_op")) return AtomicIntrinsic::Rmw;
    return AtomicIntrinsic::None;
}

MIRExprResult MIRLowerer::lower_atomic_intrinsic(FunctionCall* call, FunctionDeclaration* fd,
                                                 std::string& error) {
    const AtomicIntrinsic kind = atomic_intrinsic_of(fd);
    const uint32_t n = static_cast<uint32_t>(call->values.size());

    // An ordering / scope / op selector: the interpreter has already folded it
    // (the library passes `llvm_mem_order(order)` / `scope_to_int(...)`, both
    // comptime calls captured into the enclosing %runtime_value's refs). LLVM
    // requires these to be compile-time constants, so a runtime value is an
    // error rather than a fallback.
    auto int_arg = [&](uint32_t i, int64_t& out) -> bool {
        if (i >= n) return false;
        Value* v = call->values[i];
        if (!v) return false;
        if (v->val_kind() == ValueKind::FunctionCall) {
            auto* c = v->as_func_call_unsafe();
            if (c->captured_ref_owner && c->captured_ref_index >= 0 &&
                c->captured_ref_index <
                    static_cast<int>(c->captured_ref_owner->refs.size())) {
                Value* r = c->captured_ref_owner->refs[c->captured_ref_index].evaluated;
                if (r) v = r;
            }
        }
        if (v->val_kind() == ValueKind::IntN) {
            out = static_cast<int64_t>(v->as_int_num_value_unsafe()->get_num_value());
            return true;
        }
        if (comptime_value_eval_) {
            Value* e = comptime_value_eval_(v);
            if (e && e->val_kind() == ValueKind::IntN) {
                out = static_cast<int64_t>(e->as_int_num_value_unsafe()->get_num_value());
                return true;
            }
        }
        return false;
    };
    auto const_op = [&](int64_t bits) {
        const ConstantId c = module_.constants.add_int(MIR_INVALID_ID,
                                                       static_cast<uint64_t>(bits));
        return MIROperand::constant(c, MIR_INVALID_ID);
    };

    // Lower an argument to a scalar MIR operand. Pointers stay pointers; a
    // place that already holds the value is loaded so the emitter always sees
    // an SSA operand (MIR atomic ops take a value, not an address-of-a-local).
    auto scalar_arg = [&](uint32_t i, MIROperand& out) -> bool {
        if (i >= n) return false;
        MIRExprResult r = lower_expr(call->values[i], error);
        if (!r.ok()) return false;
        if (r.kind == MIRExprKind::Place) {
            if (r.type == MIR_INVALID_ID || r.type >= module_.types.size()) return false;
            out = MIROperand::value(builder_->load(static_cast<PlaceId>(r.id), r.type), r.type);
            return true;
        }
        if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
            out = MIROperand::value(static_cast<ValueId>(r.id), r.type);
            return true;
        }
        error = "atomic intrinsic argument produced no value";
        return false;
    };
    // Lower the memory operand and resolve what it points at (the atomic width).
    auto pointer_arg = [&](uint32_t i, MIROperand& out, TypeId& pointee) -> bool {
        if (!scalar_arg(i, out)) return false;
        const TypeId pt = out.type();
        if (pt == MIR_INVALID_ID || pt >= module_.types.size()) {
            error = "atomic intrinsic expects a pointer operand";
            return false;
        }
        const MIRTypeKind pk = module_.types.get(pt).kind;
        if (pk != MIRTypeKind::Pointer && pk != MIRTypeKind::Reference) {
            error = "atomic intrinsic expects a pointer operand";
            return false;
        }
        pointee = module_.types.get(pt).element;
        if (pointee == MIR_INVALID_ID || pointee >= module_.types.size()) {
            error = "atomic intrinsic pointer has no pointee type";
            return false;
        }
        return true;
    };

    std::vector<MIROperand> ops;
    switch (kind) {
        case AtomicIntrinsic::Fence:
        case AtomicIntrinsic::SignalFence: {
            int64_t order = 0;
            int64_t scope = 0;
            if (!int_arg(0, order)) {
                error = "atomic fence ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            if (kind == AtomicIntrinsic::Fence && !int_arg(1, scope)) {
                error = "atomic fence scope must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops.push_back(const_op(order));
            ops.push_back(const_op(scope));
            ops.push_back(const_op(kind == AtomicIntrinsic::SignalFence ? 1 : 0));
            builder_->emit(MIROpcode::AtomicFence, MIR_NULL, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::void_result();
        }
        case AtomicIntrinsic::Load: {
            MIROperand ptr;
            TypeId pointee = MIR_INVALID_ID;
            int64_t order = 0;
            int64_t scope = 0;
            if (!pointer_arg(0, ptr, pointee)) return MIRExprResult::error();
            if (!int_arg(1, order) || !int_arg(2, scope)) {
                error = "atomic load ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops = {ptr, const_op(order), const_op(scope)};
            const ValueId v = builder_->new_value(pointee);
            builder_->emit(MIROpcode::AtomicLoad, v, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::value(v, pointee);
        }
        case AtomicIntrinsic::Store: {
            MIROperand ptr;
            TypeId pointee = MIR_INVALID_ID;
            MIROperand value;
            int64_t order = 0;
            int64_t scope = 0;
            if (!pointer_arg(0, ptr, pointee)) return MIRExprResult::error();
            if (!scalar_arg(1, value)) return MIRExprResult::error();
            if (!int_arg(2, order) || !int_arg(3, scope)) {
                error = "atomic store ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops = {ptr, value, const_op(order), const_op(scope)};
            builder_->emit(MIROpcode::AtomicStore, MIR_NULL, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::void_result();
        }
        case AtomicIntrinsic::CmpXchgWeak:
        case AtomicIntrinsic::CmpXchgStrong: {
            MIROperand ptr;
            MIROperand expected;
            MIROperand value;
            TypeId pointee = MIR_INVALID_ID;
            int64_t success_order = 0;
            int64_t failure_order = 0;
            int64_t scope = 0;
            if (!pointer_arg(0, ptr, pointee)) return MIRExprResult::error();
            // `expected` is a pointer to the value to compare against; on
            // failure it receives the value that was actually there.
            if (!scalar_arg(1, expected)) return MIRExprResult::error();
            if (!scalar_arg(2, value)) return MIRExprResult::error();
            if (!int_arg(3, success_order) || !int_arg(4, failure_order) ||
                !int_arg(5, scope)) {
                error = "atomic compare-exchange ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops = {ptr, expected, value, const_op(success_order), const_op(failure_order),
                   const_op(scope),
                   const_op(kind == AtomicIntrinsic::CmpXchgWeak ? 1 : 0)};
            const TypeId bool_t = types_.bool_type();
            const ValueId v = builder_->new_value(bool_t);
            builder_->emit(MIROpcode::AtomicCmpXchg, v, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::value(v, bool_t);
        }
        case AtomicIntrinsic::Rmw: {
            int64_t op = 0;
            MIROperand ptr;
            MIROperand value;
            TypeId pointee = MIR_INVALID_ID;
            int64_t order = 0;
            int64_t scope = 0;
            if (!int_arg(0, op)) {
                error = "atomic operation must be a compile-time constant";
                return MIRExprResult::error();
            }
            if (!pointer_arg(1, ptr, pointee)) return MIRExprResult::error();
            if (!scalar_arg(2, value)) return MIRExprResult::error();
            if (!int_arg(3, order) || !int_arg(4, scope)) {
                error = "atomic operation ordering must be a compile-time constant";
                return MIRExprResult::error();
            }
            ops = {const_op(op), ptr, value, const_op(order), const_op(scope)};
            const ValueId v = builder_->new_value(pointee);
            builder_->emit(MIROpcode::AtomicRMW, v, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return MIRExprResult::value(v, pointee);
        }
        case AtomicIntrinsic::None:
            break;
    }
    error = "unrecognized atomic intrinsic '" + fd->name_str() + "'";
    return MIRExprResult::error();
}

MIRExprResult MIRLowerer::lower_expr(Value* value, std::string& error) {
    if (!value) {
        error = "null value node";
        return MIRExprResult::error();
    }

    const TypeId type = types_.map(value->getType());

    switch (value->val_kind()) {
        case ValueKind::IntN: {
            auto* n = value->as_int_num_value_unsafe();
            ConstantId c = module_.constants.add_int(type, n->get_num_value());
            return MIRExprResult::value(builder_->const_int(type, c), type);
        }
        case ValueKind::Bool: {
            auto* b = value->as_bool_unsafe();
            ConstantId c = module_.constants.add_bool(type, b->value);
            return MIRExprResult::value(builder_->const_bool(type, c), type);
        }
        case ValueKind::Float: {
            auto* f = value->as_float_unsafe();
            ConstantId c = module_.constants.add_float(type, f->value);
            return MIRExprResult::value(builder_->const_float(type, c), type);
        }
        case ValueKind::Double: {
            auto* d = value->as_double_unsafe();
            ConstantId c = module_.constants.add_double(type, d->value);
            return MIRExprResult::value(builder_->const_double(type, c), type);
        }
        case ValueKind::String: {
            auto* s = value->as_string_unsafe();
            // use the actual data size: symres rewrites `length` to the target
            // array size (for padding), which would over-read the string data
            ConstantId c = module_.constants.add_string(type, s->value.data(),
                                                        static_cast<uint32_t>(s->value.size()));
            return MIRExprResult::value(builder_->const_string(type, c), type);
        }
        case ValueKind::GenericInstIdentifier:
            // a generic function reference is relinked to its concrete
            // instantiated FunctionDecl, so it lowers like an identifier
        case ValueKind::Identifier: {
            auto* id = value->as_identifier_unsafe();
            if (Value* cv = captured_comptime_value(id->linked)) {
                return lower_expr(cv, error);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::VariantCaseVariable) {
                return lower_variant_payload(id->linked->as_variant_case_var_unsafe(), false, error);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::PatternMatchId) {
                return lower_pattern_identifier(id->linked->as_patt_match_id_unsafe(), error);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::CapturedVariable) {
                return lower_captured_variable(id->linked->as_captured_var_unsafe(), false, error);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::ForInLoopStmt) {
                auto* fl = id->linked->as_for_in_loop_unsafe();
                const PlaceId place = place_for_linked(id->linked);
                if (place == MIR_INVALID_ID) {
                    error = "for-in loop variable does not resolve to a place";
                    return MIRExprResult::error();
                }
                const TypeId pt = builder_->function().places[place].type; // `*elem`
                const ValueId pv = builder_->load(place, pt);
                if (fl->is_reference()) {
                    return MIRExprResult::value(pv, pt);
                }
                const TypeId elem = module_.types.get(pt).element;
                if (abi::is_aggregate(module_, elem)) {
                    // an aggregate element is a pointer to the element (lvalue)
                    return MIRExprResult::value(pv, pt);
                }
                return MIRExprResult::value(builder_->load_indirect(pv, elem), elem);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::FunctionDecl) {
                // a function used as a value (function pointer)
                FunctionDeclaration* fd = id->linked->as_function_unsafe();
                const SymbolId sym = intern_function(fd);
                return MIRExprResult::value(builder_->function_addr(sym, type), type);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::EnumMember) {
                auto* em = id->linked->as_enum_member();
                if (em && em->init_value) return lower_expr(em->init_value, error);
                const uint64_t idx = em ? static_cast<uint64_t>(em->get_default_index()) : 0;
                const ConstantId c = module_.constants.add_int(type, idx);
                return MIRExprResult::value(builder_->const_int(type, c), type);
            }
            if (is_implicit_field_node(id->linked) && current_self_param_) {
                // an unqualified field access inside a method resolves to
                // `self.field` (the field lives on the receiver)
                VariableIdentifier self_id(current_self_param_->name,
                                           current_self_param_->type, ZERO_LOC);
                self_id.linked = current_self_param_;
                std::vector<Value*> chain = {&self_id, id};
                return lower_member_chain(chain, 2, error);
            }
            PlaceId place = place_for_linked(id->linked);
            if (place == MIR_INVALID_ID) place = place_for_name(id->value);
            if (place != MIR_INVALID_ID) {
                if (abi::is_aggregate(module_, type)) {
                    return MIRExprResult::place(place, type);
                }
                return MIRExprResult::value(builder_->load(place, type), type);
            }
            if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
                auto* vi = id->linked->as_var_init();
                if (vi->is_comptime() && vi->value) {
                    // module-level comptime constants are inlined at their use
                    return lower_expr(vi->value, error);
                }
                const SymbolId gs = intern_global(vi);
                // an unsized array global decays to a pointer to its first element
                if (BaseType* at = id->getType(); at && at->kind() == BaseTypeKind::Array &&
                    !at->as_array_type()->has_array_size()) {
                    const ValueId addr = builder_->global_addr(gs, type);
                    return MIRExprResult::value(addr, type);
                }
                // an array global decays to a pointer to its first element
                const TypeId ptrt = (type < module_.types.size() &&
                                     module_.types.get(type).kind == MIRTypeKind::Array)
                                        ? types_.pointer_type(module_.types.get(type).element, false)
                                        : types_.pointer_type(type, false);
                const ValueId addr = builder_->global_addr(gs, ptrt);
                if (abi::is_aggregate(module_, type)) {
                    return MIRExprResult::address(addr, ptrt);
                }
                return MIRExprResult::value(builder_->load_indirect(addr, type), type);
            }
            // unresolved identifier: treat as a module-level / extern global
            {
                const std::string gname(id->value.data(), id->value.size());
                if (!gname.empty()) {
                    const SymbolId gs = intern_named_global(gname);
                    const TypeId ptrt = (type < module_.types.size() &&
                                         module_.types.get(type).kind == MIRTypeKind::Array)
                                            ? types_.pointer_type(module_.types.get(type).element, false)
                                            : types_.pointer_type(type, false);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    if (abi::is_aggregate(module_, type)) {
                        return MIRExprResult::address(addr, ptrt);
                    }
                    return MIRExprResult::value(builder_->load_indirect(addr, type), type);
                }
            }
            error = "identifier does not resolve to a local place (name '" +
                    std::string(id->value.data(), id->value.size()) + "')";
            return MIRExprResult::error();
        }
        case ValueKind::NegativeValue: {
            auto* n = value->as_negative_value_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            if (abi::is_struct_like(module_, in.type)) {
                return lower_unary_operator_overload(n->getValue(), in, "neg", n->getType(), error);
            }
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::Neg));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::NotValue: {
            auto* n = value->as_not_value_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            if (abi::is_struct_like(module_, in.type)) {
                return lower_unary_operator_overload(n->getValue(), in, "not", n->getType(), error);
            }
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::Not));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::BitwiseNot: {
            auto* n = value->as_bitwise_not_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            if (abi::is_struct_like(module_, in.type)) {
                return lower_unary_operator_overload(n->getValue(), in, "bitnot", n->getType(),
                                                     error);
            }
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::BitNot));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::Expression: {
            auto* e = value->as_expression_unsafe();
            if (e->operation == Operation::LogicalAND || e->operation == Operation::LogicalOR) {
                const TypeId bool_t = types_.bool_type();
                MIRExprResult a = lower_expr(e->firstValue, error);
                if (!a.ok()) return a;
                const PlaceId res = builder_->alloca(bool_t, MIRStorageClass::Temporary);
                builder_->store(res, a.id);
                const BlockId rhs_b = builder_->create_block();
                const BlockId end_b = builder_->create_block();
                if (e->operation == Operation::LogicalAND) {
                    builder_->cond_br(a.id, rhs_b, end_b);
                } else {
                    builder_->cond_br(a.id, end_b, rhs_b);
                }
                builder_->set_block(rhs_b);
                MIRExprResult b = lower_expr(e->secondValue, error);
                if (!b.ok()) return b;
                builder_->store(res, b.id);
                builder_->br(end_b);
                builder_->set_block(end_b);
                return MIRExprResult::value(builder_->load(res, bool_t), bool_t);
            }
            MIRExprResult lhs = lower_expr(e->firstValue, error);
            if (!lhs.ok()) return lhs;
            MIRExprResult rhs = lower_expr(e->secondValue, error);
            if (!rhs.ok()) return rhs;
            MIROpcode op;
            MIRBinaryOp bop;
            if (!map_binary(e->operation, op, bop)) {
                error = "unsupported binary operator (short-circuit/logical operators need CFG)";
                return MIRExprResult::error();
            }
            if (abi::is_struct_like(module_, lhs.type) || abi::is_struct_like(module_, rhs.type)) {
                const char* method = operator_method_name(e->operation);
                if (!method) {
                    error = "unsupported operator overload";
                    return MIRExprResult::error();
                }
                return lower_operator_overload(e->firstValue, lhs, e->secondValue, rhs, method,
                                               e->getType(), error);
            }
            ConstantId opc = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(bop));
            if (op == MIROpcode::Compare) {
                return MIRExprResult::value(builder_->compare(lhs.id, rhs.id, opc, type), type);
            }
            return MIRExprResult::value(builder_->binary(lhs.id, rhs.id, opc, type), type);
        }
        case ValueKind::CastedValue: {
            auto* c = value->as_casted_value_unsafe();
            MIRExprResult in = lower_expr(c->value, error);
            if (!in.ok()) return in;
            if (type != MIR_INVALID_ID && type < module_.types.size() &&
                module_.types.get(type).kind == MIRTypeKind::Array) {
                // casting a pointer to an array: copy the bytes into a temp array
                const PlaceId tmp = builder_->alloca(type, MIRStorageClass::Temporary);
                if (in.kind == MIRExprKind::Value || in.kind == MIRExprKind::Address) {
                    builder_->copy_ptr_to_place(tmp, static_cast<ValueId>(in.id));
                } else if (in.kind == MIRExprKind::Place) {
                    const TypeId pt = builder_->function().places[in.id].type;
                    const ValueId ptr = builder_->address_of(
                        static_cast<PlaceId>(in.id), types_.pointer_type(pt, false));
                    builder_->copy_ptr_to_place(tmp, ptr);
                }
                return MIRExprResult::place(tmp, type);
            }
            if (in.kind == MIRExprKind::Place) {
                // a cast of an aggregate place to a pointer/interface type
                // (`value as StaticInterface`) yields the place's address
                if (type != MIR_INVALID_ID && type < module_.types.size()) {
                    const MIRTypeKind tk = module_.types.get(type).kind;
                    if (tk == MIRTypeKind::Pointer || tk == MIRTypeKind::Opaque) {
                        const TypeId pt = builder_->function().places[in.id].type;
                        const MIRTypeKind pk = module_.types.get(pt).kind;
                        if (pk == MIRTypeKind::Pointer || pk == MIRTypeKind::Reference) {
                            // the place already holds a pointer (an array/pointer
                            // param decayed to a pointer, or a pointer/reference
                            // local): cast the pointer *value*, not its address
                            const ValueId loaded = builder_->load(
                                static_cast<PlaceId>(in.id), pt);
                            return MIRExprResult::value(builder_->cast(loaded, type), type);
                        }
                        const ValueId addr = builder_->address_of(
                            static_cast<PlaceId>(in.id), types_.pointer_type(pt, true));
                        return MIRExprResult::value(builder_->cast(addr, type), type);
                    }
                }
                return in;
            }
            if (in.type == type) return in;
            return MIRExprResult::value(builder_->cast(in.id, type), type);
        }
        case ValueKind::AccessChain: {
            auto* chain = value->as_access_chain_unsafe();
            if (chain->values.size() == 1) {
                return lower_expr(chain->values[0], error);
            }
            // enum member access: `EnumName.Member` -> its constant value
            if (ASTNode* lk = chain->linked_node()) {
                if (lk->kind() == ASTNodeKind::EnumMember) {
                    auto* em = lk->as_enum_member();
                    if (em && em->init_value) return lower_expr(em->init_value, error);
                    const uint64_t idx =
                        em ? static_cast<uint64_t>(em->get_default_index()) : 0;
                    const ConstantId c = module_.constants.add_int(type, idx);
                    return MIRExprResult::value(builder_->const_int(type, c), type);
                }
            }
            // static method reference: `Type::method` -> a function pointer
            if (chain->values.size() >= 2) {
                if (ASTNode* lk = chain->values.back()->linked_node()) {
                    if (lk->kind() == ASTNodeKind::VarInitStmt) {
                        // qualified module-level constant/global: resolve the leaf
                        return lower_expr(chain->values.back(), error);
                    }
                    if (lk->kind() == ASTNodeKind::FunctionDecl) {
                        auto* fd = lk->as_function();
                        ASTNode* base = chain->values[0]->linked_node();
                        const bool base_is_type =
                            base && (base->kind() == ASTNodeKind::StructDecl ||
                                     base->kind() == ASTNodeKind::ImplDecl ||
                                     base->kind() == ASTNodeKind::InterfaceDecl ||
                                     base->kind() == ASTNodeKind::NamespaceDecl ||
                                     base->kind() == ASTNodeKind::VariantDecl ||
                                     base->kind() == ASTNodeKind::UnionDecl ||
                                     base->kind() == ASTNodeKind::EnumDecl ||
                                     base->kind() == ASTNodeKind::GenericStructDecl);
                        if (base_is_type) {
                            const SymbolId sym = intern_function(fd);
                            return MIRExprResult::value(builder_->function_addr(sym, type),
                                                        type);
                        }
                    }
                }
            }
            // a member access on a comptime-known global (e.g. `def.is64Bit`)
            if (comptime_value_eval_ && chain->values.size() >= 2) {
                Value* b = chain->values[0];
                if (b && b->val_kind() == ValueKind::AccessChain) {
                    auto* c = b->as_access_chain_unsafe();
                    if (c->values.size() == 1) b = c->values[0];
                }
                ASTNode* bl = b ? b->linked_node() : nullptr;
                if (bl && bl->kind() == ASTNodeKind::VarInitStmt &&
                    bl->as_var_init()->is_comptime()) {
                    Value* ev = comptime_value_eval_(value);
                    if (ev && ev != value) return lower_expr(ev, error);
                }
            }
            // member access: walk the chain from the base, taking the address of
            // each intermediate field and loading the last
            return lower_member_chain(chain->values, chain->values.size(), error);
        }
        case ValueKind::StructValue: {
            auto* sv = value->as_struct_value_unsafe();
            TypeId st = type;
            StructDefinition* sd = nullptr;
            UnionDef* ud = nullptr;
            UnnamedStruct* us = nullptr;
            UnnamedUnion* uu = nullptr;
            StructType* stc = nullptr;
            auto bind_container = [&](BaseType* ctype) {
                if (!ctype) return;
                ASTNode* cn = ctype->get_direct_linked_canonical_node();
                if (!cn) cn = ctype->get_direct_linked_node();
                if (!cn) return;
                if (cn->kind() == ASTNodeKind::StructDecl) {
                    sd = cn->as_struct_def_unsafe();
                } else if (cn->kind() == ASTNodeKind::UnionDecl) {
                    ud = cn->as_union_def_unsafe();
                } else if (cn->kind() == ASTNodeKind::UnnamedStruct) {
                    us = cn->as_unnamed_struct_unsafe();
                } else if (cn->kind() == ASTNodeKind::UnnamedUnion) {
                    uu = cn->as_unnamed_union_unsafe();
                } else if (cn->kind() == ASTNodeKind::StructType) {
                    stc = static_cast<StructType*>(cn);
                }
            };
            bind_container(value->getType());
            auto unresolved = [&]() -> bool {
                return (!sd && !ud && !us && !uu && !stc) || st == MIR_INVALID_ID ||
                       st >= module_.types.size() ||
                       module_.types.get(st).kind == MIRTypeKind::Opaque;
            };
            if (unresolved() && expected_struct_type_) {
                st = types_.map(expected_struct_type_);
                sd = nullptr;
                ud = nullptr;
                us = nullptr;
                stc = nullptr;
                bind_container(expected_struct_type_);
            }
            if (unresolved()) {
                // e.g. a generic struct literal whose instantiation MIR has not
                // resolved: fall back rather than emit `void*` field stores
                error = "struct literal with an unresolved (generic?) type is not yet supported";
                return MIRExprResult::error();
            }
            auto lookup_member_type = [&](const chem::string_view& nm) -> BaseType* {
                if (sd) {
                    auto r = sd->variable_type_w_index(nm);
                    if (r.second != -1) return r.first;
                }
                if (ud) {
                    for (BaseDefMember* v : ud->variables()) {
                        if (v && v->name == nm) return v->known_type();
                    }
                }
                if (us) {
                    for (BaseDefMember* v : us->variables()) {
                        if (v && v->name == nm) return v->known_type();
                    }
                }
                if (uu) {
                    for (BaseDefMember* v : uu->variables()) {
                        if (v && v->name == nm) return v->known_type();
                    }
                }
                if (stc) {
                    auto r = stc->variable_type_w_index(nm);
                    if (r.second != -1) return r.first;
                }
                return nullptr;
            };
            const PlaceId temp = builder_->alloca(st, MIRStorageClass::Temporary);
            if (temp == MIR_INVALID_ID) {
                error = "failed to allocate struct temporary";
                return MIRExprResult::error();
            }
            // The names the literal sets explicitly, computed before emission so
            // the zero_init can be skipped when nothing is left unset.
            std::set<std::string> explicit_fields;
            for (auto& kv : sv->values) {
                explicit_fields.insert(std::string(kv.first.data(), kv.first.size()));
            }
            // Fields the literal *fully* defines. An array literal shorter than
            // the field's declared size lowers to a temp of exactly the literal's
            // length and copies only those bytes (the empty `[]` case is handled
            // by the array lowering, but a partial one decays), so the field's
            // tail is defined only by the enclosing zero_init -- treat such a
            // field as unset for the zero decision.
            std::set<std::string> zero_defined_fields = explicit_fields;
            for (auto& kv : sv->values) {
                Value* iv = kv.second.value;
                if (!iv || iv->val_kind() != ValueKind::ArrayValue) continue;
                BaseType* ft = nullptr;
                auto mtw = sv->child_type_w_index(kv.first);
                if (mtw.second != -1) ft = mtw.first;
                auto* fat = ft ? ft->as_array_type() : nullptr;
                if (!fat || !fat->has_array_size()) continue;
                if (static_cast<uint64_t>(iv->as_array_value_unsafe()->values.size()) <
                    fat->get_array_size()) {
                    zero_defined_fields.erase(
                        std::string(kv.first.data(), kv.first.size()));
                }
            }
            // Zero the temp so members that are not explicitly set are
            // well-defined (matches the legacy backend's default initialization)
            // -- but only when a member really is left unset. When the literal
            // defines every member (or the rest default-construct / take a
            // default value), the whole-temp zero is dead bytes the legacy
            // backend never emitted (evaluation §1.2, action C1).
            const bool all_defined =
                sd && struct_literal_defines_all_fields(sd, zero_defined_fields);
            if (!all_defined) {
                builder_->zero_init(temp);
            }
            auto it = sv->values.begin();
            while (it != sv->values.end()) {
                auto& init = it.value();
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, init.name.data(), static_cast<uint32_t>(init.name.size()));
                // implicit conversion: if the member type has an implicit
                // constructor for the initializer value, construct into the field
                FunctionDeclaration* implicit = nullptr;
                BaseType* member_type = nullptr;
                {
                    auto mtw = sv->child_type_w_index(init.name);
                    if (mtw.second != -1) {
                        member_type = mtw.first;
                        ASTNode* ln = member_type->get_direct_linked_canonical_node();
                        if (ln && ln->kind() == ASTNodeKind::StructDecl) {
                            implicit = ln->as_struct_def_unsafe()->implicit_constructor_func(init.value);
                        }
                        if (!implicit) implicit = member_type->implicit_constructor_for(init.value);
                    }
                }
                // an anonymous union/struct literal field initializer has no
                // spellable C type for a temporary: initialize the field in place
                if (member_type && init.value &&
                    init.value->val_kind() == ValueKind::StructValue) {
                    ASTNode* mn = member_type->get_direct_linked_canonical_node();
                    if (mn && (mn->kind() == ASTNodeKind::UnnamedUnion ||
                               mn->kind() == ASTNodeKind::UnnamedStruct)) {
                        const TypeId mt = types_.map(member_type);
                        const ValueId faddr = builder_->field_addr(temp, fc, mt);
                        if (!store_anon_aggregate(faddr, init.value->as_struct_value_unsafe(),
                                                  error)) {
                            return MIRExprResult::error();
                        }
                        ++it;
                        continue;
                    }
                }
                if (implicit) {
                    if (implicit->is_comptime() && comptime_ctor_eval_) {
                        Value* evaluated = comptime_ctor_eval_(implicit, init.value);
                        if (evaluated) {
                            MIRExprResult er = lower_expr(evaluated, error);
                            if (!er.ok()) return er;
                            if (er.kind == MIRExprKind::Place) {
                                builder_->field_store_place(temp, fc, static_cast<PlaceId>(er.id));
                            } else if (er.kind == MIRExprKind::Value) {
                                builder_->field_store(temp, fc, static_cast<ValueId>(er.id));
                            } else {
                                error = "comptime constructor did not produce a value";
                                return MIRExprResult::error();
                            }
                            ++it;
                            continue;
                        }
                    }
                    const TypeId mt = types_.map(member_type);
                    const PlaceId tmp = builder_->alloca(mt, MIRStorageClass::Temporary);
                    std::vector<MIROperand> cargs;
                    std::vector<Value*> one{init.value};
                    if (!lower_call_args(one, cargs, error)) return MIRExprResult::error();
                    const SymbolId sym = intern_function(implicit);
                    builder_->zero_init(tmp);
                    builder_->init(tmp, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                    builder_->field_store_place(temp, fc, tmp);
                    ++it;
                    continue;
                }
                MIRExprResult r = member_type
                                      ? lower_arg_converted(init.value, member_type, error)
                                      : lower_expr(init.value, error);
                if (!r.ok()) return r;
                // an array initializer for a pointer/reference field decays to a
                // pointer to its first element (never a byte copy, which would
                // overrun the pointer-sized field)
                if (r.kind == MIRExprKind::Place && member_type) {
                    BaseType* mtc = member_type->canonical();
                    BaseType* it0 = init.value ? init.value->getType() : nullptr;
                    BaseType* atc = it0 ? it0->canonical() : nullptr;
                    if (mtc && (mtc->kind() == BaseTypeKind::Pointer ||
                                mtc->kind() == BaseTypeKind::Reference) &&
                        atc && atc->kind() == BaseTypeKind::Array) {
                        const TypeId ptr_t = types_.map(member_type);
                        builder_->field_store(
                            temp, fc, builder_->address_of(static_cast<PlaceId>(r.id), ptr_t));
                        ++it;
                        continue;
                    }
                }
                if ((r.kind == MIRExprKind::Place || r.kind == MIRExprKind::Address) &&
                    member_type) {
                    BaseType* mtc = member_type->canonical();
                    if (mtc && (mtc->kind() == BaseTypeKind::Pointer ||
                                mtc->kind() == BaseTypeKind::Reference)) {
                        // a pointer/reference field takes a pointer: the RHS
                        // pointer value, or the address of an aggregate RHS (the
                        // emitter derefs a pointer-typed place RHS, which would
                        // be wrong here)
                        ValueId pv = MIR_NULL;
                        if (r.kind == MIRExprKind::Address) {
                            pv = static_cast<ValueId>(r.id);
                        } else {
                            const TypeId pt = builder_->function().places[r.id].type;
                            const MIRTypeRecord& pr = module_.types.get(pt);
                            pv = (pr.kind == MIRTypeKind::Pointer ||
                                  pr.kind == MIRTypeKind::Reference)
                                     ? builder_->load(static_cast<PlaceId>(r.id), pt)
                                     : builder_->address_of(static_cast<PlaceId>(r.id),
                                                            types_.pointer_type(pt, true));
                        }
                        builder_->field_store(temp, fc, pv);
                        ++it;
                        continue;
                    }
                }
                if (r.kind == MIRExprKind::Place) {
                    const PlaceId src = static_cast<PlaceId>(r.id);
                    builder_->field_store_place(temp, fc, src);
                    // a destructible local moved into the struct must not be
                    // destroyed again at scope exit (unwraps the single-element
                    // AccessChain the parser usually produces)
                    Value* v0 = init.value;
                    if (v0 && v0->val_kind() == ValueKind::AccessChain) {
                        auto* ch = v0->as_access_chain_unsafe();
                        if (ch->values.size() == 1) v0 = ch->values[0];
                    }
                    if (v0 && v0->val_kind() == ValueKind::Identifier) {
                        BaseType* at = v0->getType();
                        BaseType* canon = at ? at->canonical() : nullptr;
                        if (!(canon && canon->kind() == BaseTypeKind::Array)) {
                            mark_moved(src);
                        }
                    }
                    ++it;
                    continue;
                }
                if (r.kind == MIRExprKind::Address) {
                    // an lvalue address as a field initializer: load the pointee
                    if (!member_type) member_type = lookup_member_type(init.name);
                    const TypeId ft = member_type ? types_.map(member_type) : r.type;
                    builder_->field_store(
                        temp, fc,
                        builder_->load_indirect(static_cast<ValueId>(r.id), ft));
                    ++it;
                    continue;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "unsupported struct field initializer";
                    return MIRExprResult::error();
                }
                // if the field is an aggregate value but the initializer is a
                // pointer/reference, dereference to get the value
                if (!member_type) {
                    member_type = lookup_member_type(init.name);
                }
                if (member_type) {
                    const MIRTypeRecord& rt = module_.types.get(r.type);
                    if ((rt.kind == MIRTypeKind::Pointer ||
                         rt.kind == MIRTypeKind::Reference) &&
                        abi::is_aggregate(module_, types_.map(member_type))) {
                        builder_->field_store(
                            temp, fc,
                            builder_->load_indirect(r.id, types_.map(member_type)));
                        ++it;
                        continue;
                    }
                }
                builder_->field_store(temp, fc, r.id);
                ++it;
            }
            // initialize members that were not explicitly set with their default
            // values, and default-construct inherited bases
            if (sd) {
                ExtendableMembersContainerNode* defc =
                    sd ? static_cast<ExtendableMembersContainerNode*>(sd)
                       : (ud ? static_cast<ExtendableMembersContainerNode*>(ud) : nullptr);
                if (defc && !apply_struct_defaults(temp, defc, explicit_fields, error)) {
                    return MIRExprResult::error();
                }
            }
            return MIRExprResult::place(temp, st);
        }
        case ValueKind::FunctionCall: {
            auto* call = value->as_func_call_unsafe();
            // `intrinsics::llvm::atomic_*` lower to first-class MIR atomic ops.
            // This must precede the captured-comptime-ref shortcut: the atomic
            // intrinsics are comptime calls, so inside a `%runtime_value(...)`
            // the interpreter records the *call node itself* as the ref's
            // evaluated value (their operands resolve through their own captured
            // refs), and interpreting them here would require a live LLVM
            // IRBuilder and a real `llvm::Value` for each runtime pointer.
            if (FunctionDeclaration* afd = call->safe_linked_func()) {
                if (afd->is_compiler_decl() &&
                    atomic_intrinsic_of(afd) != AtomicIntrinsic::None) {
                    return lower_atomic_intrinsic(call, afd, error);
                }
            }
            if (call->captured_ref_owner && call->captured_ref_index >= 0 &&
                call->captured_ref_index <
                    static_cast<int>(call->captured_ref_owner->refs.size())) {
                Value* ev =
                    call->captured_ref_owner->refs[call->captured_ref_index].evaluated;
                if (ev) return lower_expr(ev, error);
            }
            FunctionDeclaration* fd = nullptr;
            std::vector<MIROperand> args;

            // variant construction: `Variant.Member(args)` / `ns.Variant.Member(args)`
            {
                ASTNode* vleaf = nullptr;
                if (call->parent_val) {
                    if (call->parent_val->val_kind() == ValueKind::AccessChain) {
                        auto* ch = call->parent_val->as_access_chain_unsafe();
                        if (!ch->values.empty()) vleaf = ch->values.back()->linked_node();
                    } else {
                        vleaf = call->parent_val->linked_node();
                    }
                }
                if (vleaf && vleaf->kind() == ASTNodeKind::VariantMember) {
                    auto* vm = vleaf->as_variant_member_unsafe();
                    VariantDefinition* vd = vm->parent();
                    const TypeId vt = types_.map(call->getType());
                    const PlaceId temp = builder_->alloca(vt, MIRStorageClass::Temporary);
                    // The whole-temp zero defines the bytes the active case does
                    // not write -- but the only bytes ever read for a variant case
                    // are its discriminator and its own payload, so the zero is
                    // dead once every payload field is written (the union tail is
                    // never read). It is still needed when a payload parameter is
                    // omitted without a default, a payload is a partially-written
                    // array, or the variant has an inherited base. The legacy
                    // backend emitted no zero at all; see action C1 in
                    // mir-migration-evaluation.md.
                    bool payload_defined = true;
                    {
                        size_t pi = 0;
                        for (auto& entry : vm->values) {
                            VariantMemberParam* param = entry.second;
                            Value* arg = nullptr;
                            if (pi < call->values.size()) {
                                arg = call->values[pi];
                                pi++;
                            } else if (param) {
                                arg = param->def_value;
                            }
                            if (!arg) {
                                payload_defined = false;
                                break;
                            }
                            BaseType* pt = param ? param->type : nullptr;
                            BaseType* canon = pt ? pt->canonical() : nullptr;
                            if (canon && canon->kind() == BaseTypeKind::Array) {
                                // a partial array literal defines only its own
                                // bytes; the tail depends on the zero
                                payload_defined = false;
                                break;
                            }
                        }
                    }
                    if (!(payload_defined && vd && vd->inherited.empty())) {
                        builder_->zero_init(temp);
                    }
                    const std::string tag_name("__chx__vt_621827");
                    const ConstantId tag_fc = module_.constants.add_string(
                        MIR_INVALID_ID, tag_name.data(), static_cast<uint32_t>(tag_name.size()));
                    const TypeId it = types_.u32_type();
                    const long idx = vd ? vd->direct_child_index(vm->name) : 0;
                    const ValueId tag = builder_->const_int(
                        it, module_.constants.add_int(
                                it, static_cast<uint64_t>(idx < 0 ? 0 : idx)));
                    builder_->field_store(temp, tag_fc, tag);
                    size_t ai = 0;
                    for (auto& entry : vm->values) {
                        VariantMemberParam* param = entry.second;
                        Value* arg = nullptr;
                        if (ai < call->values.size()) {
                            arg = call->values[ai];
                            ai++;
                        } else if (param) {
                            // an omitted parameter with a default value
                            arg = param->def_value;
                        }
                        if (!arg) continue;
                        const std::string mname(vm->name.data(), vm->name.size());
                        const std::string pname(entry.first.data(), entry.first.size());
                        const std::string fname = mname + "." + pname;
                        const ConstantId fc = module_.constants.add_string(
                            MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                        MIRExprResult r = lower_arg_converted(
                            arg, param ? param->type : nullptr, error);
                        if (!r.ok()) return r;
                        if (r.kind == MIRExprKind::Place) {
                            const PlaceId src = static_cast<PlaceId>(r.id);
                            builder_->field_store_place(temp, fc, src);
                            // a destructible local moved into the variant must not
                            // be destroyed again at scope exit (mirrors the
                            // function-call argument move handling). The argument
                            // is often a single-element AccessChain wrapping the
                            // identifier, so unwrap it first.
                            Value* v0 = arg;
                            if (v0 && v0->val_kind() == ValueKind::AccessChain) {
                                auto* ch = v0->as_access_chain_unsafe();
                                if (ch->values.size() == 1) v0 = ch->values[0];
                            }
                            if (v0 && v0->val_kind() == ValueKind::Identifier) {
                                BaseType* at = v0->getType();
                                BaseType* canon = at ? at->canonical() : nullptr;
                                if (!(canon && canon->kind() == BaseTypeKind::Array)) {
                                    mark_moved(src);
                                }
                            }
                        } else if (r.kind == MIRExprKind::Address) {
                            const TypeId ft = param && param->type ? types_.map(param->type) : r.type;
                            const ValueId loaded =
                                builder_->load_indirect(static_cast<ValueId>(r.id), ft);
                            builder_->field_store(temp, fc, loaded);
                        } else {
                            builder_->field_store(temp, fc, static_cast<ValueId>(r.id));
                        }
                    }
                    if (!apply_struct_defaults(temp, vd, {}, error)) {
                        return MIRExprResult::error();
                    }
                    return MIRExprResult::place(temp, vt);
                }
            }

            // aggregate construction: `Type(args)` / `ns.Type(args)` /
            // `ns.Generic<...>(args)`
            ASTNode* ctor_node = nullptr;
            if (call->parent_val) {
                // a call whose parent is itself a call is a function-pointer call,
                // not a constructor: `parent->linked_node()` would be the inner
                // call's *return type* (a struct), which is misleading
                const bool parent_is_call =
                    call->parent_val->val_kind() == ValueKind::FunctionCall;
                ASTNode* plk = parent_is_call ? nullptr : call->parent_val->linked_node();
                if (plk && plk->as_struct_def()) {
                    ctor_node = plk;
                } else if (call->parent_val->val_kind() == ValueKind::AccessChain) {
                    auto* ch = call->parent_val->as_access_chain_unsafe();
                    if (!ch->values.empty()) {
                        ASTNode* lk = ch->values.back()->linked_node();
                        if (lk && (lk->as_struct_def() || lk->kind() == ASTNodeKind::GenericStructDecl)) {
                            ctor_node = lk;
                        }
                    }
                }
            }
            if (!ctor_node && call->getType() && call->parent_val &&
                call->parent_val->val_kind() == ValueKind::AccessChain) {
                // unlinked constructor call (`std::string_view(...)`): match the
                // callee name against the call's aggregate type name
                auto* ch = call->parent_val->as_access_chain_unsafe();
                std::string mname, merr;
                if (!ch->values.empty() && member_name(ch->values.back(), mname, merr)) {
                    ASTNode* cn = call->getType()->get_direct_linked_canonical_node();
                    if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                        auto* sd = cn->as_struct_def_unsafe();
                        if (sd->name_view() ==
                            chem::string_view(mname.data(), static_cast<unsigned>(mname.size()))) {
                            ctor_node = cn;
                        }
                    }
                }
            }
            if (ctor_node) {
                BaseType* ctype = call->getType();
                const TypeId st = types_.map(ctype);
                const PlaceId place = builder_->alloca(st, MIRStorageClass::Temporary);
                StructDefinition* sd = nullptr;
                if (ctype) {
                    ASTNode* cn = ctype->get_direct_linked_canonical_node();
                    if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                        sd = cn->as_struct_def_unsafe();
                    }
                }
                if (!sd) sd = ctor_node->as_struct_def();
                FunctionDeclaration* ctor = sd ? sd->constructor_func(call->values) : nullptr;
                if (!ctor && ctype && call->values.empty()) ctor = ctype->get_def_constructor();
                // evaluate a comptime constructor before lowering its arguments
                // (they may be comptime-only values such as expressive strings)
                if (ctor && ctor->is_comptime() && comptime_eval_) {
                    Value* ev = comptime_eval_(call, ctor);
                    if (ev) return lower_expr(ev, error);
                }
                std::vector<MIROperand> cargs;
                // constructors have no `self` parameter in `params` (the result
                // is passed through the hidden sret pointer)
                const bool ctor_self = ctor && ctor->get_self_param() != nullptr;
                if (!lower_call_args_for(ctor, ctor_self, call->values, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && !append_default_args(ctor, call->values.size(), ctor_self, cargs,
                                                 error)) {
                    return MIRExprResult::error();
                }
                if (ctor) {
                    const SymbolId sym = intern_function(ctor);
                    // the result temp's zero is dead when the construction writes
                    // every member: a `return T{...}` constructor's result is a
                    // whole-value copy of a fully defined temporary, and a
                    // generated constructor's body is exactly
                    // apply_struct_defaults_ptr over the container
                    // (evaluation §1.2, action C1)
                    ExtendableMembersContainerNode* cdef =
                        ctor->parent() ? ctor->parent()->as_extendable_member_container() : nullptr;
                    const bool covered = constructor_writes_whole_value(ctor) ||
                        (ctor->is_generated_fn() && default_init_writes_all_fields(cdef));
                    if (!covered) {
                        builder_->zero_init(place);
                    }
                    builder_->init(place, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                } else if (!cargs.empty()) {
                    error = "no matching constructor for aggregate construction";
                    return MIRExprResult::error();
                }
                return MIRExprResult::place(place, st);            }

            // method call: `receiver.method(args)`
            if (call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* chain = call->parent_val->as_access_chain_unsafe();
                if (chain->values.size() >= 2) {
                    // resolve the callee first: a namespace-qualified function
                    // (e.g. `lab::curr_dir()`) must not be mistaken for a
                    // constructor even when it returns an aggregate
                    ASTNode* callee_node = call->parent_val->linked_node();
                    FunctionDeclaration* mfd = callee_node ? callee_node->as_function() : nullptr;
                    if (!mfd) {
                        // fallback: the method identifier may not have been linked
                        // (extension methods on an interface receiver); resolve it
                        // by name on the receiver's canonical container.
                        std::string mname, merr;
                        if (member_name(chain->values.back(), mname, merr)) {
                            mfd = resolve_method_fallback(chain->values[0]->getType(), mname);
                        }
                    }
                    // namespace-qualified aggregate construction: the callee is
                    // not a function, the receiver is not an addressable place,
                    // and the result is an aggregate
                    std::string probe;
                    const bool recv_is_place =
                        resolve_place(chain->values[0], probe) != MIR_INVALID_ID;
                    const TypeId ct = types_.map(call->getType());
                    // a struct-member callee is a field call (`st.task()`), never
                    // an aggregate construction -- the receiver may be a captured
                    // variable (not a place), which must not trigger this path
                    const bool callee_is_field =
                        callee_node && callee_node->kind() == ASTNodeKind::StructMember;
                    if (!mfd && !recv_is_place && !callee_is_field &&
                        abi::is_aggregate(module_, ct)) {
                        const PlaceId place = builder_->alloca(ct, MIRStorageClass::Temporary);
                        FunctionDeclaration* ctor = nullptr;
                        if (call->getType()) {
                            ASTNode* cn = call->getType()->get_direct_linked_canonical_node();
                            if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                                ctor = cn->as_struct_def_unsafe()->constructor_func(call->values);
                            }
                        }
                        if (!ctor && call->getType() && call->values.empty()) {
                            ctor = call->getType()->get_def_constructor();
                        }
                        std::vector<MIROperand> cargs;
                        const bool ctor_self = ctor && ctor->get_self_param() != nullptr;
                        if (!lower_call_args_for(ctor, ctor_self, call->values, cargs, error)) {
                            return MIRExprResult::error();
                        }
                        if (ctor && !append_default_args(ctor, call->values.size(), ctor_self, cargs,
                                                         error)) {
                            return MIRExprResult::error();
                        }
                        if (ctor && ctor->is_comptime() && comptime_eval_) {
                            Value* ev = comptime_eval_(call, ctor);
                            if (ev) return lower_expr(ev, error);
                        }
                        if (ctor) {
                            const SymbolId sym = intern_function(ctor);
                            // as above: a `return T{...}` constructor writes the
                            // whole result, and a generated constructor covers
                            // every field exactly when default init does (C1)
                            ExtendableMembersContainerNode* cdef =
                                ctor->parent() ? ctor->parent()->as_extendable_member_container()
                                               : nullptr;
                            const bool covered = constructor_writes_whole_value(ctor) ||
                                (ctor->is_generated_fn() &&
                                 default_init_writes_all_fields(cdef));
                            if (!covered) {
                                builder_->zero_init(place);
                            }
                            builder_->init(place, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                        } else if (!cargs.empty()) {
                            error = "no matching constructor for aggregate construction";
                            return MIRExprResult::error();
                        }
                        return MIRExprResult::place(place, ct);
                    }
                    const bool needs_receiver =
                        mfd && (mfd->get_self_param() != nullptr || mfd->isExtensionFn());
                    if (needs_receiver) {
                        const size_t n = chain->values.size();
                        MIRExprResult recv = lower_receiver(chain->values, n - 1, error);
                        if (!recv.ok()) return recv;
                        BaseType* rtype = chain->values[n - 2]->getType();
                        return lower_method_call(recv, rtype, call, error);
                    }
                    // a non-self method called on a call receiver: evaluate the
                    // receiver for its side effects (no receiver is passed)
                    if (!needs_receiver && chain->values[0] &&
                        chain->values[0]->val_kind() == ValueKind::FunctionCall) {
                        MIRExprResult recv = lower_expr(chain->values[0], error);
                        if (!recv.ok()) return recv;
                        maybe_register_call_temp(chain->values[0], recv);
                    }
                    // otherwise: namespace/type-qualified free function or a
                    // static method -- fall through to the direct-call path,
                    // which passes no receiver.
                }
            }
            {
                // when the callee is itself a call (`f()()` / `factory.create()()`),
                // the outer call goes through the result, not the inner function
                const bool parent_is_call =
                    call->parent_val && call->parent_val->val_kind() == ValueKind::FunctionCall;
                ASTNode* linked_fn =
                    (!parent_is_call && call->parent_val) ? call->parent_val->linked_node()
                                                          : nullptr;
                fd = linked_fn ? linked_fn->as_function() : nullptr;
                if (!fd) {
                    // `obj.field(args)` where `field` is a capturing function
                    // (`std::function`) member: dispatch through the closure's
                    // fn/data pointers. The access-chain's type can resolve to
                    // the *inner* function signature inside a generic
                    // instantiation, so key on the member's declared type.
                    if (linked_fn && linked_fn->kind() == ASTNodeKind::StructMember &&
                        call->parent_val &&
                        call->parent_val->val_kind() == ValueKind::AccessChain) {
                        auto* sm = linked_fn->as_struct_member_unsafe();
                        BaseType* ft_ast = sm ? sm->type : nullptr;
                        BaseType* ftc = ft_ast ? ft_ast->canonical() : nullptr;
                        bool field_is_cap =
                            ftc && (ftc->kind() == BaseTypeKind::CapturingFunction ||
                                    ftc->get_cap_func_type() != nullptr);
                        if (!field_is_cap && ftc) {
                            ASTNode* cln = ftc->get_direct_linked_canonical_node();
                            if (cln && cln->kind() == ASTNodeKind::StructDecl) {
                                auto* sd = cln->as_struct_def_unsafe();
                                field_is_cap = sd->child("get_fn_ptr") != nullptr &&
                                               sd->child("get_data_ptr") != nullptr;
                            }
                        }
                        if (field_is_cap) {
                            MIRExprResult fpr = lower_expr(call->parent_val, error);
                            if (!fpr.ok()) return fpr;
                            return lower_capturing_function_call(fpr, ft_ast, call, error);
                        }
                    }
                    // function-pointer variable/parameter call -> call_indirect
                    PlaceId fp = linked_fn ? place_for_linked(linked_fn) : MIR_INVALID_ID;
                    if (fp == MIR_INVALID_ID && call->parent_val &&
                        call->parent_val->val_kind() == ValueKind::Identifier) {
                        fp = place_for_linked(call->parent_val->as_identifier_unsafe()->linked);
                    }
                    if (fp == MIR_INVALID_ID) {
                        // a call through a capturing function instance that is not
                        // a plain place (e.g. `make_fn()()`): lower the receiver
                        // expression and dispatch through it indirectly
                        if (call->parent_val && call->parent_val->getType()) {
                            BaseType* rt = call->parent_val->getType();
                            BaseType* rtc = rt->canonical();
                            bool is_cap = rtc && rtc->get_cap_func_type() != nullptr;
                            if (!is_cap && rtc) {
                                ASTNode* cln = rtc->get_direct_linked_canonical_node();
                                if (cln && cln->kind() == ASTNodeKind::StructDecl) {
                                    auto* sd = cln->as_struct_def_unsafe();
                                    is_cap = sd->child("get_fn_ptr") != nullptr &&
                                             sd->child("get_data_ptr") != nullptr;
                                }
                            }
                            if (is_cap) {
                                MIRExprResult recv = lower_expr(call->parent_val, error);
                                if (!recv.ok()) return recv;
                                return lower_capturing_function_call(recv, rt, call, error);
                            }
                            // a call returning a plain function pointer:
                            // `factory.create_lamb()()`
                            if (!is_cap && rtc && rtc->kind() == BaseTypeKind::Function) {
                                MIRExprResult fpr = lower_expr(call->parent_val, error);
                                if (!fpr.ok()) return fpr;
                                const ValueId fnptr =
                                    (fpr.kind == MIRExprKind::Place)
                                        ? builder_->load(static_cast<PlaceId>(fpr.id), fpr.type)
                                        : static_cast<ValueId>(fpr.id);
                                FunctionType* ft_fn = rtc->get_canonical_function_type();
                                std::vector<BaseType*> ast_params;
                                if (ft_fn) {
                                    for (FunctionParam* p : ft_fn->params) {
                                        ast_params.push_back(p ? p->type : nullptr);
                                    }
                                }
                                std::vector<MIROperand> cargs;
                                const size_t cmark = call_temps_.size();
                                if (!lower_call_args_for(nullptr, false, call->values, cargs,
                                                         error, &ast_params)) {
                                    return MIRExprResult::error();
                                }
                                const TypeId crt = types_.map(call->getType());
                                if (is_void_type(module_, crt)) {
                                    builder_->call_indirect(fnptr, crt, cargs.data(),
                                                            static_cast<uint32_t>(cargs.size()));
                                    destroy_call_temps_from(cmark, error);
                                    return MIRExprResult::void_result();
                                }
                                if (abi::needs_sret(module_, crt)) {
                                    const PlaceId tmp =
                                        builder_->alloca(crt, MIRStorageClass::Temporary);
                                    builder_->zero_init(tmp);
                                    builder_->call_indirect_sret(
                                        fnptr, tmp, cargs.data(),
                                        static_cast<uint32_t>(cargs.size()));
                                    destroy_call_temps_from(cmark, error);
                                    return MIRExprResult::place(tmp, crt);
                                }
                                const ValueId v = builder_->call_indirect(
                                    fnptr, crt, cargs.data(),
                                    static_cast<uint32_t>(cargs.size()));
                                destroy_call_temps_from(cmark, error);
                                return MIRExprResult::value(v, crt);
                            }
                        }
                        // `obj.field(args)` where `field` is a function-typed
                        // struct member: load the function pointer and dispatch
                        if (linked_fn && linked_fn->kind() == ASTNodeKind::StructMember &&
                            call->parent_val &&
                            call->parent_val->val_kind() == ValueKind::AccessChain) {
                            auto* sm = linked_fn->as_struct_member_unsafe();
                            BaseType* ft_ast = sm ? sm->type : nullptr;
                            std::vector<BaseType*> ast_params;
                            FunctionType* ft_fn =
                                ft_ast ? ft_ast->canonical()->get_canonical_function_type()
                                       : nullptr;
                            if (ft_fn) {
                                for (FunctionParam* p : ft_fn->params) {
                                    ast_params.push_back(p ? p->type : nullptr);
                                }
                            }
                            MIRExprResult fpr = lower_expr(call->parent_val, error);
                            if (!fpr.ok()) return fpr;
                            if (fpr.type != MIR_INVALID_ID && fpr.type < module_.types.size() &&
                                module_.types.get(fpr.type).kind == MIRTypeKind::Struct) {
                                return lower_capturing_function_call(fpr, ft_ast, call, error);
                            }
                            const ValueId fnptr =
                                (fpr.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(fpr.id), fpr.type)
                                    : static_cast<ValueId>(fpr.id);
                            std::vector<MIROperand> cargs;
                            const size_t cmark = call_temps_.size();
                            if (!lower_call_args_for(nullptr, false, call->values, cargs, error,
                                                     &ast_params)) {
                                return MIRExprResult::error();
                            }
                            const TypeId rt = types_.map(call->getType());
                            if (is_void_type(module_, rt)) {
                                builder_->call_indirect(fnptr, rt, cargs.data(),
                                                        static_cast<uint32_t>(cargs.size()));
                                destroy_call_temps_from(cmark, error);
                                return MIRExprResult::void_result();
                            }
                            if (abi::needs_sret(module_, rt)) {
                                const PlaceId tmp =
                                    builder_->alloca(rt, MIRStorageClass::Temporary);
                                builder_->zero_init(tmp);
                                builder_->call_indirect_sret(
                                    fnptr, tmp, cargs.data(),
                                    static_cast<uint32_t>(cargs.size()));
                                destroy_call_temps_from(cmark, error);
                                return MIRExprResult::place(tmp, rt);
                            }
                            const ValueId v =
                                builder_->call_indirect(fnptr, rt, cargs.data(),
                                                        static_cast<uint32_t>(cargs.size()));
                            destroy_call_temps_from(cmark, error);
                            return MIRExprResult::value(v, rt);
                        }
                        std::string callee;
                        int last_linked_kind = -2;
                        int pv_kind = -1;
                        if (call->parent_val) {
                            Value* pv = call->parent_val;
                            pv_kind = static_cast<int>(pv->val_kind());
                            if (pv->val_kind() == ValueKind::AccessChain) {
                                auto* ch = pv->as_access_chain_unsafe();
                                if (!ch->values.empty()) {
                                    std::string n, e;
                                    if (member_name(ch->values.back(), n, e)) callee = n;
                                    ASTNode* ll = ch->values.back()->linked_node();
                                    last_linked_kind = ll ? static_cast<int>(ll->kind()) : -1;
                                }
                            } else if (pv->val_kind() == ValueKind::Identifier) {
                                auto* id = pv->as_identifier_unsafe();
                                callee.assign(id->value.data(), id->value.size());
                                last_linked_kind = id->linked ? static_cast<int>(id->linked->kind()) : -1;
                            }
                        }
                        error = "call to unresolved function '" + callee + "' (linked kind " +
                                std::to_string(linked_fn
                                                   ? static_cast<int>(linked_fn->kind())
                                                   : -1) +
                                ", pv kind " + std::to_string(pv_kind) + ", last linked " +
                                std::to_string(last_linked_kind) + ")";
                        return MIRExprResult::error();
                    }
                    const TypeId ft = builder_->function().places[fp].type;
                    // AST parameter types of the function pointer (to know which
                    // arguments are by-value vs by-reference)
                    std::vector<BaseType*> ast_params;
                    {
                        BaseType* fp_ast = nullptr;
                        if (linked_fn) {
                            if (linked_fn->kind() == ASTNodeKind::FunctionParam) {
                                fp_ast = linked_fn->as_func_param_unsafe()->type;
                            } else if (linked_fn->kind() == ASTNodeKind::VarInitStmt) {
                                fp_ast = linked_fn->as_var_init()->known_type();
                            }
                        }
                        if (!fp_ast && call->parent_val) fp_ast = call->parent_val->getType();
                        if (fp_ast) {
                            FunctionType* ft_ast = fp_ast->canonical()->get_canonical_function_type();
                            if (ft_ast) {
                                for (FunctionParam* p : ft_ast->params) {
                                    ast_params.push_back(p ? p->type : nullptr);
                                }
                            }
                        }
                    }
                    if (ft != MIR_INVALID_ID && ft < module_.types.size()) {
                        const MIRTypeRecord& ftr = module_.types.get(ft);
                        const MIRTypeRecord* eff = &ftr;
                        if ((ftr.kind == MIRTypeKind::Pointer ||
                             ftr.kind == MIRTypeKind::Reference) &&
                            ftr.element < module_.types.size()) {
                            eff = &module_.types.get(ftr.element);
                        }
                        if (eff->kind == MIRTypeKind::Struct) {
                            // capturing function instance (`std::function`) call
                            BaseType* recv_ast =
                                call->parent_val ? call->parent_val->getType() : nullptr;
                            return lower_capturing_function_call(MIRExprResult::place(fp, ft),
                                                                 recv_ast, call, error);
                        }
                    }
                    const ValueId fnptr = builder_->load(fp, ft);
                    std::vector<MIROperand> cargs;
                    const size_t cmark = call_temps_.size();
                    if (!lower_call_args_for(nullptr, false, call->values, cargs, error,
                                             &ast_params)) {
                        return MIRExprResult::error();
                    }
                    const TypeId rt = types_.map(call->getType());
                    if (is_void_type(module_, rt)) {
                        builder_->call_indirect(fnptr, rt, cargs.data(), static_cast<uint32_t>(cargs.size()));
                        destroy_call_temps_from(cmark, error);
                        return MIRExprResult::void_result();
                    }
                    if (abi::needs_sret(module_, rt)) {
                        // aggregate return: use the sret convention (the callee
                        // returns void and writes through a leading pointer)
                        const PlaceId tmp = builder_->alloca(rt, MIRStorageClass::Temporary);
                        builder_->zero_init(tmp);
                        builder_->call_indirect_sret(fnptr, tmp, cargs.data(),
                                                     static_cast<uint32_t>(cargs.size()));
                        destroy_call_temps_from(cmark, error);
                        return MIRExprResult::place(tmp, rt);
                    }
                    const ValueId v = builder_->call_indirect(fnptr, rt, cargs.data(),
                                                              static_cast<uint32_t>(cargs.size()));
                    destroy_call_temps_from(cmark, error);
                    return MIRExprResult::value(v, rt);
                }
            }
            // an unqualified method call inside a method (`reserve(n)`) is a
            // call on the implicit receiver `self`
            if (fd && fd->has_self_param() && current_self_param_ && call->parent_val &&
                call->parent_val->val_kind() == ValueKind::Identifier) {
                VariableIdentifier self_id(current_self_param_->name,
                                           current_self_param_->type, ZERO_LOC);
                self_id.linked = current_self_param_;
                MIRExprResult recv = lower_expr(&self_id, error);
                if (!recv.ok()) return recv;
                return lower_method_call(recv, current_self_param_->type, call, error);
            }
            // `intrinsics::forget(x)` clears x's drop flag (the value is
            // deliberately not destroyed at scope exit); it has no runtime
            // effect, so model it directly instead of via the comptime value
            if (fd->is_compiler_decl() && fd->name_view() == chem::string_view("forget") &&
                call->values.size() == 1) {
                PlaceId p = resolve_place(call->values.front(), error);
                if (p != MIR_INVALID_ID) mark_moved(p);
                return MIRExprResult::void_result();
            }
            if (fd->is_comptime() && comptime_eval_) {
                Value* evaluated = comptime_eval_(call, fd);
                if (!evaluated) {
                    // a void comptime call whose evaluation produced no value
                    // (e.g. an inline-asm fence) is a no-op
                    if (is_void_type(module_, type)) return MIRExprResult::void_result();
                    error = "comptime call to '" + fd->name_str() + "' could not be evaluated";
                    return MIRExprResult::error();
                }
                return lower_expr(evaluated, error);
            }
            // a non-self interface default called as `receiver.method(...)` is
            // still emitted per implementing struct
            BaseType* direct_recv_type = nullptr;
            if (call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
                auto* dch = call->parent_val->as_access_chain_unsafe();
                if (dch->values.size() >= 2) {
                    direct_recv_type = dch->values[dch->values.size() - 2]->getType();
                }
            }
            SymbolId sym = intern_method_function(fd, direct_recv_type);

            args.reserve(args.size() + call->values.size());
            const size_t cmark = call_temps_.size();
            if (!lower_call_args_for(fd, false, call->values, args, error)) {
                return MIRExprResult::error();
            }
            if (!append_default_args(fd, call->values.size(), false, args, error)) {
                return MIRExprResult::error();
            }

            if (is_void_type(module_, type)) {
                builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
                destroy_call_temps_from(cmark, error);
                return MIRExprResult::void_result();
            }
            if (abi::needs_sret(module_, type)) {
                const PlaceId res = builder_->alloca(type, MIRStorageClass::Temporary);
                if (res == MIR_INVALID_ID) {
                    error = "failed to allocate struct-return result place";
                    return MIRExprResult::error();
                }
                builder_->call_sret(sym, res, args.data(), static_cast<uint32_t>(args.size()));
                destroy_call_temps_from(cmark, error);
                return MIRExprResult::place(res, type);
            }
            ValueId v = builder_->call_scalar(sym, type, args.data(), static_cast<uint32_t>(args.size()));
            destroy_call_temps_from(cmark, error);
            return MIRExprResult::value(v, type);
        }
        case ValueKind::AddrOfValue: {
            auto* a = value->as_addr_of_value_unsafe();
            return lower_address_of(a->value, error);
        }
        case ValueKind::ReferenceOfValue: {
            auto* r = value->as_reference_of_value_unsafe();
            // `&x`: an aggregate parameter passed as a hidden pointer yields its
            // referent; everything else takes the address of the expression.
            Value* inner = r->value;
            if (inner && inner->val_kind() == ValueKind::AccessChain) {
                auto* c = inner->as_access_chain_unsafe();
                if (c->values.size() == 1) inner = c->values[0];
            }
            if (inner && inner->val_kind() == ValueKind::Identifier) {
                auto* id = inner->as_identifier_unsafe();
                ASTNode* ln = id->linked;
                bool hidden = false;
                if (ln && ln->kind() == ASTNodeKind::FunctionParam) {
                    BaseType* pt = ln->as_func_param_unsafe()->type;
                    hidden = pt && pt->kind() != BaseTypeKind::Dynamic &&
                             pt->isStructLikeType();
                }
                if (!hidden) {
                    PlaceId p = place_for_linked(ln);
                    if (p == MIR_INVALID_ID) p = place_for_name(id->value);
                    if (p != MIR_INVALID_ID) {
                        const TypeId ptrt = types_.pointer_type(
                            builder_->function().places[p].type, true);
                        return MIRExprResult::value(builder_->address_of(p, ptrt), ptrt);
                    }
                }
            }
            return lower_address_of(r->value, error);
        }
        case ValueKind::ArrayValue: {
            auto* arr = value->as_array_value_unsafe();
            TypeId at = type;
            bool decay = false;
            TypeId elem_t = MIR_INVALID_ID;
            // an empty literal `[]` in a pointer context is a null pointer
            if (arr->values.empty() && at != MIR_INVALID_ID && at < module_.types.size() &&
                (module_.types.get(at).kind == MIRTypeKind::Pointer ||
                 module_.types.get(at).kind == MIRTypeKind::Reference)) {
                const ConstantId c = module_.constants.add_int(at, 0);
                return MIRExprResult::value(builder_->const_null(at, c), at);
            }
            if (at == MIR_INVALID_ID || at >= module_.types.size() ||
                module_.types.get(at).kind != MIRTypeKind::Array) {
                // an unsized array literal (`const x = [...]`, a pointer context)
                // decays to a pointer to its first element: build a sized array
                // temporary for the literal and return its address
                BaseType* elem_bt = arr->known_elem_type();
                if (elem_bt) elem_t = types_.map(elem_bt);
                if (elem_t == MIR_INVALID_ID ||
                    static_cast<uint32_t>(arr->values.size()) == 0) {
                    error = "array literal without an array type";
                    return MIRExprResult::error();
                }
                at = types_.sized_array_type(elem_t,
                                             static_cast<uint32_t>(arr->values.size()));
                decay = true;
            }
            const PlaceId place = builder_->alloca(at, MIRStorageClass::Temporary);
            // Zero the array so elements not covered by the literal are 0 -- but
            // only when the literal leaves tail elements. A literal that fills
            // the array (the common case, including the decayed unsized form,
            // which is sized to the literal) writes every element, so the
            // whole-array zero is dead bytes the legacy backend never emitted
            // (evaluation §1.2, action C1).
            const bool full_literal =
                at != MIR_INVALID_ID && at < module_.types.size() &&
                module_.types.get(at).kind == MIRTypeKind::Array &&
                arr->values.size() >=
                    static_cast<size_t>(module_.types.get(at).data_count);
            if (!full_literal) {
                builder_->zero_init(place);
            }
            const TypeId ut = types_.u32_type();
            BaseType* elem_bt = arr->known_elem_type();
            for (size_t i = 0; i < arr->values.size(); ++i) {
                MIRExprResult e = lower_arg_converted(arr->values[i], elem_bt, error);
                if (!e.ok()) return e;
                ValueId val = MIR_NULL;
                if (e.kind == MIRExprKind::Place) {
                    const PlaceId pid = static_cast<PlaceId>(e.id);
                    const TypeId pt = builder_->function().places[pid].type;
                    const ValueId idx = builder_->const_int(
                        ut, module_.constants.add_int(ut, static_cast<uint64_t>(i)));
                    if (abi::is_aggregate(module_, pt)) {
                        builder_->element_store_place(place, idx, pid);
                        // a destructible local moved into the array must not be
                        // destroyed again at scope exit
                        Value* ev = arr->values[i];
                        if (ev && ev->val_kind() == ValueKind::AccessChain) {
                            auto* ch = ev->as_access_chain_unsafe();
                            if (ch->values.size() == 1) ev = ch->values[0];
                        }
                        if (ev && ev->val_kind() == ValueKind::Identifier) {
                            BaseType* at = ev->getType();
                            BaseType* canon = at ? at->canonical() : nullptr;
                            if (!(canon && canon->kind() == BaseTypeKind::Array)) {
                                mark_moved(pid);
                            }
                        }
                        continue;
                    }
                    val = builder_->load(pid, pt);
                    builder_->element_store(place, idx, val);
                    continue;
                }
                const ValueId idx = builder_->const_int(
                    ut, module_.constants.add_int(ut, static_cast<uint64_t>(i)));
                if (e.kind == MIRExprKind::Address) {
                    const TypeId et = types_.map(elem_bt);
                    if (abi::is_aggregate(module_, et)) {
                        // `*ptr` element: copy the pointee into the element
                        builder_->element_store(place, idx,
                                                builder_->load_indirect(e.id, et));
                    } else {
                        builder_->element_store(place, idx, static_cast<ValueId>(e.id));
                    }
                    continue;
                }
                if (e.kind == MIRExprKind::Value) {
                    val = static_cast<ValueId>(e.id);
                } else {
                    error = "unsupported array element expression";
                    return MIRExprResult::error();
                }
                builder_->element_store(place, idx, val);
            }
            if (decay) {
                const TypeId ptrt = types_.pointer_type(elem_t, true);
                return MIRExprResult::value(builder_->address_of(place, ptrt), ptrt);
            }
            return MIRExprResult::place(place, at);
        }
        case ValueKind::IndexOperator: {
            auto* io = value->as_index_op_unsafe();
            MIRExprResult base = lower_expr(io->parent_val, error);
            if (!base.ok()) return base;
            maybe_register_call_temp(io->parent_val, base);
            MIRExprResult idx = lower_expr(io->idx, error);
            if (!idx.ok()) return idx;
            // a reference used as an index is automatically dereferenced
            if (idx.type != MIR_INVALID_ID && idx.type < module_.types.size() &&
                module_.types.get(idx.type).kind == MIRTypeKind::Reference) {
                const TypeId it = module_.types.get(idx.type).element;
                const ValueId pv = (idx.kind == MIRExprKind::Place)
                                       ? builder_->load(static_cast<PlaceId>(idx.id), idx.type)
                                       : static_cast<ValueId>(idx.id);
                idx = MIRExprResult::value(builder_->load_indirect(pv, it), it);
            }
            if (base.kind == MIRExprKind::Place) {
                const TypeId bt = builder_->function().places[base.id].type;
                if (abi::is_struct_like(module_, bt)) {
                    // `x[i]` on a struct uses the `Index`/`IndexMut` method
                    BaseType* recv_t = io->parent_val ? io->parent_val->getType() : nullptr;
                    FunctionDeclaration* fd = resolve_method_fallback(recv_t, "index");
                    if (!fd) fd = resolve_method_fallback(recv_t, "index_mut");
                    if (!fd) {
                        error = "index operator overloading is not yet supported";
                        return MIRExprResult::error();
                    }
                    VariableIdentifier op_id(chem::string_view("index"), nullptr, ZERO_LOC);
                    op_id.linked = fd;
                    FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
                    op_call.values.push_back(io->idx);
                    MIRExprResult r = lower_method_call(base, recv_t, &op_call, error);
                    if (!r.ok()) return r;
                    // the method returns `&Output`: dereference to the element
                    if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
                        if (abi::is_aggregate(module_, type)) {
                            return MIRExprResult::address(static_cast<ValueId>(r.id), type);
                        }
                        return MIRExprResult::value(
                            builder_->load_indirect(static_cast<ValueId>(r.id), type), type);
                    }
                    return r;
                }
            }
            if (abi::is_aggregate(module_, type)) {
                // aggregate element: return its address (an lvalue), not a load
                const TypeId ptrt = types_.pointer_type(type, true);
                if (base.kind == MIRExprKind::Place) {
                    return MIRExprResult::address(
                        builder_->index_addr(static_cast<PlaceId>(base.id),
                                             static_cast<ValueId>(idx.id), type), type);
                }
                if (base.kind == MIRExprKind::Value || base.kind == MIRExprKind::Address) {
                    return MIRExprResult::address(
                        builder_->index_addr_ptr(static_cast<ValueId>(base.id),
                                                 static_cast<ValueId>(idx.id), type), type);
                }
                error = "index base is not a place or pointer value";
                return MIRExprResult::error();
            }
            if (base.kind == MIRExprKind::Place) {
                return MIRExprResult::value(
                    builder_->element_load(static_cast<PlaceId>(base.id),
                                           static_cast<ValueId>(idx.id), type), type);
            }
            if (base.kind == MIRExprKind::Value || base.kind == MIRExprKind::Address) {
                return MIRExprResult::value(
                    builder_->index_load(static_cast<ValueId>(base.id),
                                         static_cast<ValueId>(idx.id), type), type);
            }
            error = "index base is not a place or pointer value";
            return MIRExprResult::error();
        }
        case ValueKind::NullValue: {
            const ConstantId c = module_.constants.add_null(type);
            return MIRExprResult::value(builder_->const_null(type, c), type);
        }
        case ValueKind::DereferenceValue: {
            auto* d = value->as_dereference_value_unsafe();
            MIRExprResult in = lower_expr(d->getValue(), error);
            if (!in.ok()) return in;
            if (in.kind != MIRExprKind::Value && in.kind != MIRExprKind::Address) {
                error = "dereference operand is not a pointer value";
                return MIRExprResult::error();
            }
            if (abi::is_aggregate(module_, type)) {
                return MIRExprResult::address(static_cast<ValueId>(in.id), in.type);
            }
            return MIRExprResult::value(
                builder_->load_indirect(static_cast<ValueId>(in.id), type), type);
        }
        case ValueKind::IncDecValue: {
            auto* n = value->as_inc_dec_value_unsafe();
            return lower_incdec_value(n->getValue(), n->increment, n->post, error);
        }
        case ValueKind::PatternMatchExpr: {
            return lower_pattern_match_value(value->as_pattern_match_expr_unsafe(), error);
        }
        case ValueKind::ExtractionValue:
            return lower_extraction_value(static_cast<ExtractionValue*>(value), error);
        case ValueKind::NewTypedValue: {
            // `new T` -> `malloc(sizeof(T))`
            auto* nt = static_cast<NewTypedValue*>(value);
            BaseType* inner = const_cast<BaseType*>(nt->type.getType());
            const TypeId inner_t = types_.map(inner);
            const TypeId u64t = types_.u64_type();
            const TypeId ptr_t = types_.pointer_type(inner_t, true);
            const ValueId size_v = builder_->size_of(inner_t, u64t);
            return MIRExprResult::value(emit_malloc(size_v, u64t, ptr_t), ptr_t);
        }
        case ValueKind::NewValue: {
            // `new value` -> allocate, then store the value into the allocation
            auto* nv = static_cast<NewValue*>(value);
            BaseType* vt_ast = nv->value ? nv->value->getType() : nullptr;
            const TypeId vt = types_.map(vt_ast);
            const TypeId u64t = types_.u64_type();
            const TypeId ptr_t = types_.pointer_type(vt, true);
            const ValueId size_v = builder_->size_of(vt, u64t);
            const ValueId mem = emit_malloc(size_v, u64t, ptr_t);
            MIRExprResult r = lower_expr(nv->value, error);
            if (!r.ok()) return r;
            if (r.kind == MIRExprKind::Place) {
                const TypeId rt = builder_->function().places[r.id].type;
                builder_->store_indirect(mem, builder_->load(static_cast<PlaceId>(r.id), rt));
            } else if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
                builder_->store_indirect(mem, static_cast<ValueId>(r.id));
            } else {
                error = "new value does not produce a value";
                return MIRExprResult::error();
            }
            return MIRExprResult::value(mem, ptr_t);
        }
        case ValueKind::PlacementNewValue: {
            // `new (ptr) value` -> store the value through `ptr`
            auto* pn = static_cast<PlacementNewValue*>(value);
            MIRExprResult pp = lower_expr(pn->pointer, error);
            if (!pp.ok()) return pp;
            ValueId mem = (pp.kind == MIRExprKind::Place)
                              ? builder_->load(static_cast<PlaceId>(pp.id), pp.type)
                              : static_cast<ValueId>(pp.id);
            MIRExprResult r = lower_expr(pn->value, error);
            if (!r.ok()) return r;
            if (r.kind == MIRExprKind::Place) {
                const TypeId rt = builder_->function().places[r.id].type;
                builder_->store_indirect(mem, builder_->load(static_cast<PlaceId>(r.id), rt));
            } else if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
                builder_->store_indirect(mem, static_cast<ValueId>(r.id));
            } else {
                error = "placement new value does not produce a value";
                return MIRExprResult::error();
            }
            return MIRExprResult::value(mem, pp.type);
        }
        case ValueKind::RuntimeBlockValue:
        case ValueKind::BlockValue: {
            // a `{ ... }` / `%runtime_block_value { ... }` statement-expression:
            // its trailing value (or symres-calculated value) is the result
            Scope* scope = nullptr;
            Value* calculated = nullptr;
            if (value->val_kind() == ValueKind::RuntimeBlockValue) {
                scope = &static_cast<RuntimeBlockValue*>(value)->scope;
            } else {
                auto* bv = static_cast<BlockValue*>(value);
                scope = &bv->scope;
                calculated = bv->getCalculatedValue();
            }
            const TypeId vt = type;
            const PlaceId dest = builder_->alloca(vt, MIRStorageClass::Temporary);
            if (calculated) {
                const size_t mark = destructibles_.size();
                for (ASTNode* n : scope->nodes) {
                    if (builder_->current_block_terminated()) break;
                    if (!lower_stmt(n, error)) return MIRExprResult::error();
                }
                MIRExprResult r = lower_expr(calculated, error);
                if (!r.ok()) return r;
                if (r.kind == MIRExprKind::Place) {
                    builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), vt));
                } else if (r.kind == MIRExprKind::Address) {
                    builder_->store(dest,
                                    builder_->load_indirect(static_cast<ValueId>(r.id), vt));
                } else if (r.kind == MIRExprKind::Value) {
                    builder_->store(dest, static_cast<ValueId>(r.id));
                } else {
                    error = "block value does not produce a value";
                    return MIRExprResult::error();
                }
                if (!builder_->current_block_terminated()) {
                    emit_drops_from(mark);
                }
                destructibles_.resize(mark);
            } else if (!lower_scope_value(*scope, dest, vt, error)) {
                return MIRExprResult::error();
            }
            if (abi::is_aggregate(module_, vt)) {
                return MIRExprResult::place(dest, vt);
            }
            return MIRExprResult::value(builder_->load(dest, vt), vt);
        }
        case ValueKind::InValue: {
            // `x in a, b, c` -> a chain of equality tests (or `!in` -> not-equal)
            auto* in = static_cast<InValue*>(value);
            const TypeId bool_t = types_.bool_type();
            MIRExprResult v = lower_expr(in->value, error);
            if (!v.ok()) return v;
            const PlaceId res = builder_->alloca(bool_t, MIRStorageClass::Temporary);
            builder_->store(
                res, builder_->const_bool(
                         bool_t, module_.constants.add_bool(bool_t, in->is_negating)));
            const BlockId end_b = builder_->create_block();
            const ConstantId eqc = module_.constants.add_int(
                MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));
            for (Value* cv : in->values) {
                MIRExprResult c = lower_expr(cv, error);
                if (!c.ok()) return c;
                const ValueId cond = builder_->compare(v.id, c.id, eqc, bool_t);
                const BlockId hit_b = builder_->create_block();
                const BlockId next_b = builder_->create_block();
                builder_->cond_br(cond, hit_b, next_b);
                builder_->set_block(hit_b);
                builder_->store(res,
                                builder_->const_bool(
                                    bool_t, module_.constants.add_bool(bool_t, !in->is_negating)));
                builder_->br(end_b);
                builder_->set_block(next_b);
            }
            builder_->br(end_b);
            builder_->set_block(end_b);
            return MIRExprResult::value(builder_->load(res, bool_t), bool_t);
        }
        case ValueKind::IsValue: {
            auto* isv = value->as_is_value_unsafe();
            ASTNode* ln = isv->type ? isv->type->get_direct_linked_node() : nullptr;
            if (ln && ln->kind() == ASTNodeKind::VariantMember) {
                auto* vm = ln->as_variant_member_unsafe();
                VariantDefinition* vd = vm->parent();
                MIRExprResult v = lower_expr(isv->value, error);
                if (!v.ok()) return v;
                const std::string tag_name("__chx__vt_621827");
                const ConstantId tag_fc = module_.constants.add_string(
                    MIR_INVALID_ID, tag_name.data(), static_cast<uint32_t>(tag_name.size()));
                const TypeId it = types_.u32_type();
                ValueId tag = MIR_NULL;
                if (v.kind == MIRExprKind::Place) {
                    tag = builder_->field_load(static_cast<PlaceId>(v.id), tag_fc, it);
                } else if (v.kind == MIRExprKind::Address || v.kind == MIRExprKind::Value) {
                    tag = builder_->field_load_ptr(static_cast<ValueId>(v.id), tag_fc, it);
                } else {
                    error = "is-check operand is not a variant lvalue";
                    return MIRExprResult::error();
                }
                const long idx = vd ? vd->direct_child_index(vm->name) : 0;
                const ValueId c = builder_->const_int(
                    it, module_.constants.add_int(
                            it, static_cast<uint64_t>(idx < 0 ? 0 : idx)));
                const TypeId bool_t = types_.bool_type();
                const ConstantId pred = module_.constants.add_int(
                    MIR_INVALID_ID,
                    static_cast<uint64_t>(isv->is_negating ? MIRBinaryOp::Ne : MIRBinaryOp::Eq));
                return MIRExprResult::value(builder_->compare(tag, c, pred, bool_t), bool_t);
            }
            if (std::optional<bool> comp = isv->get_comp_time_result(); comp.has_value()) {
                const TypeId bool_t = types_.bool_type();
                return MIRExprResult::value(
                    builder_->const_bool(
                        bool_t, module_.constants.add_bool(bool_t, comp.value())),
                    bool_t);
            }
            error = "unsupported is-expression (non-variant)";
            return MIRExprResult::error();
        }
        case ValueKind::IfValue: {
            return lower_if_value(value->as_if_value_unsafe(), error);
        }
        case ValueKind::SwitchValue: {
            return lower_switch_value(value->as_switch_value_unsafe(), error);
        }
        case ValueKind::LoopValue: {
            return lower_loop_value(value->as_loop_value_unsafe(), error);
        }
        case ValueKind::SizeOfValue: {
            auto* s = value->as_sizeof_value_unsafe();
            BaseType* ft = s->for_type.getType()
                               ? resolve_sizeof_operand(const_cast<BaseType*>(s->for_type.getType()))
                               : nullptr;
            const TypeId tid = ft ? types_.map(ft) : types_.opaque_type();
            return MIRExprResult::value(builder_->size_of(tid, type), type);
        }
        case ValueKind::AlignOfValue: {
            auto* a = static_cast<AlignOfValue*>(value);
            BaseType* ft = a->for_type.getType()
                               ? resolve_sizeof_operand(const_cast<BaseType*>(a->for_type.getType()))
                               : nullptr;
            const TypeId tid = ft ? types_.map(ft) : types_.opaque_type();
            return MIRExprResult::value(builder_->align_of(tid, type), type);
        }
        case ValueKind::OffsetOfValue: {
            auto* o = value->as_offset_of_value_unsafe();
            BaseType* ft = o->for_type.getType()
                               ? resolve_sizeof_operand(const_cast<BaseType*>(o->for_type.getType()))
                               : nullptr;
            const TypeId tid = ft ? types_.map(ft) : types_.opaque_type();
            const std::string mn(o->member_name.data(), o->member_name.size());
            const ConstantId fc =
                module_.constants.add_string(MIR_INVALID_ID, mn.data(),
                                            static_cast<uint32_t>(mn.size()));
            return MIRExprResult::value(builder_->offset_of(tid, fc, type), type);
        }
        case ValueKind::UnsafeValue: {
            // `unsafe(expr)` is a compile-time safety marker only
            auto* u = static_cast<UnsafeValue*>(value);
            if (u->getValue()) return lower_expr(u->getValue(), error);
            error = "unsafe value has no inner expression";
            return MIRExprResult::error();
        }
        case ValueKind::ZeroedValue: {
            const TypeId zt = type;
            const PlaceId tmp = builder_->alloca(zt, MIRStorageClass::Temporary);
            builder_->zero_init(tmp);
            if (abi::is_aggregate(module_, zt)) return MIRExprResult::place(tmp, zt);
            return MIRExprResult::value(builder_->load(tmp, zt), zt);
        }
        case ValueKind::RuntimeValue:
            return lower_expr(static_cast<RuntimeValue*>(value)->underlying, error);
        case ValueKind::DynamicValue: {
            auto* dv = static_cast<DynamicValue*>(value);
            BaseType* iface_t = dv->getInterfaceType();
            ASTNode* iface_node =
                iface_t ? iface_t->get_direct_linked_canonical_node() : nullptr;
            if (!iface_node || !vtable_mangler_) {
                error = "dynamic value interface could not be resolved";
                return MIRExprResult::error();
            }
            const std::string vt =
                vtable_mangler_(iface_node, dv->value ? dv->value->getType() : nullptr);
            if (vt.empty()) {
                error = "dynamic value vtable name is empty";
                return MIRExprResult::error();
            }
            const TypeId ft = types_.fat_pointer_type();
            const TypeId voidp = types_.pointer_type(types_.opaque_type(), true);
            const PlaceId temp = builder_->alloca(ft, MIRStorageClass::Temporary);
            // A fat pointer is `{ void* first; void* second; }` with no padding,
            // and both fields are written just below -- the whole-temp zero was
            // dead bytes (evaluation §1.2, action C1).
            ValueId obj_ptr = MIR_NULL;
            // A fat pointer is an *escaping* address: it routinely outlives the
            // call that produced it (that is the whole point of `ret : dyn I`).
            // For a by-value aggregate parameter the parameter place is a copy
            // living in this frame, so pointing at it would hand the caller (or
            // anyone else) a dangling pointer once we return. Use the argument
            // pointer instead — the caller's object — exactly like the
            // interpreter (which shares the value) and the pre-copy MIR model
            // did (`&param` of an aggregate param is documented as the pointer).
            FunctionParam* agg_param = aggregate_param_operand(dv->value);
            if (agg_param) {
                obj_ptr = aggregate_param_ptrs_[agg_param];
            } else {
                MIRExprResult obj = lower_address_of(dv->value, error);
                if (!obj.ok()) return obj;
                if (obj.kind == MIRExprKind::Place) {
                    const TypeId pt = builder_->function().places[obj.id].type;
                    obj_ptr = builder_->address_of(obj.id, types_.pointer_type(pt, true));
                } else if (obj.kind == MIRExprKind::Value || obj.kind == MIRExprKind::Address) {
                    obj_ptr = static_cast<ValueId>(obj.id);
                } else {
                    error = "dynamic value operand is not addressable";
                    return MIRExprResult::error();
                }
            }
            const ConstantId first_c =
                module_.constants.add_string(MIR_INVALID_ID, "first", 5);
            builder_->field_store(temp, first_c, obj_ptr);
            const SymbolId gs = intern_named_global(vt);
            const ValueId vt_addr = builder_->global_addr(gs, voidp);
            const ConstantId second_c =
                module_.constants.add_string(MIR_INVALID_ID, "second", 6);
            builder_->field_store(temp, second_c, vt_addr);
            return MIRExprResult::place(temp, ft);
        }
        case ValueKind::ComptimeValue:
            return lower_expr(static_cast<ComptimeValue*>(value)->getValue(), error);
        case ValueKind::LambdaFunc: {
            return lower_lambda(value->as_lambda_func_unsafe(), error);
        }
        case ValueKind::AwaitExpr: {
            // a non-suspending (transparent) await: use the operand's value.
            // Suspending awaits are hoisted into VarInitStatements by the async
            // normalization pass and handled by `async_lower_await_var_init`.
            auto* aw = value->as_await_expression_unsafe();
            if (!aw || !aw->getInner()) {
                error = "await expression has no operand";
                return MIRExprResult::error();
            }
            return lower_expr(aw->getInner(), error);
        }
        case ValueKind::EmbeddedValue: {
            auto* ev = value->as_embedded_value_unsafe();
            Value* repl = nullptr;
            auto it = embedded_value_cache_.find(ev);
            if (it != embedded_value_cache_.end()) {
                repl = it->second;
            } else if (embedded_value_repl_) {
                repl = embedded_value_repl_(ev);
                if (repl) embedded_value_cache_[ev] = repl;
            }
            if (!repl) {
                error = "embedded value has no replacement";
                return MIRExprResult::error();
            }
            return lower_expr(repl, error);
        }
        default:
            error = "unsupported value kind during MIR lowering (kind " +
                    std::to_string(static_cast<int>(value->val_kind())) + ")";
            return MIRExprResult::error();
    }
}

bool MIRLowerer::lower_asm_operand(Value* expr, MIROperand& out, std::string& error) {
    Value* t = expr;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t) {
        error = "inline asm operand is null";
        return false;
    }
    // a memory operand `*ptr`: keep the pointer value; the emitter spells `*<ptr>`
    if (t->val_kind() == ValueKind::DereferenceValue) {
        auto* d = t->as_dereference_value_unsafe();
        MIRExprResult p = lower_expr(d->getValue(), error);
        if (!p.ok()) return false;
        if (p.kind != MIRExprKind::Value && p.kind != MIRExprKind::Address) {
            error = "inline asm dereference operand is not a pointer";
            return false;
        }
        out = MIROperand::value(static_cast<ValueId>(p.id), p.type);
        return true;
    }
    // a direct local / parameter / member lvalue
    std::string perr;
    const PlaceId place = resolve_place_or_lower(t, perr);
    if (place != MIR_INVALID_ID) {
        const TypeId pt = builder_->function().places[place].type;
        out = MIROperand::place(place, pt);
        return true;
    }
    // fall back to its address (field / index lvalues)
    MIRExprResult a = lower_address_of(t, error);
    if (a.ok() && (a.kind == MIRExprKind::Value || a.kind == MIRExprKind::Address)) {
        out = MIROperand::value(static_cast<ValueId>(a.id), a.type);
        return true;
    }
    error = "unsupported inline asm operand: " + perr;
    return false;
}

void MIRLowerer::note_source(ASTNode* node) {
    if (!builder_ || !source_loc_provider_ || !node) return;
    const uint64_t enc = node->encoded_location().encoded;
    if (enc == 0) return;
    MIRSourceLoc loc;
    if (!source_loc_provider_(enc, loc)) return;
    builder_->set_source(builder_->add_source(loc));
}

bool MIRLowerer::lower_stmt(ASTNode* node, std::string& error) {
    if (!node) return true;
    note_source(node);

    switch (node->kind()) {
        case ASTNodeKind::VarInitStmt: {
            auto* vi = node->as_var_init();
            TypeId vt = types_.map(vi->known_type());
            PlaceId place = MIR_INVALID_ID;
            if (async_) {
                const std::string rf = async_resident_field(vi);
                if (!rf.empty()) {
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, rf.data(), static_cast<uint32_t>(rf.size()));
                    place = builder_->alloca_frame(vt, fc);
                }
            }
            if (place == MIR_INVALID_ID) place = builder_->alloca(vt, MIRStorageClass::Local);
            if (place == MIR_INVALID_ID) {
                error = "failed to allocate place for variable";
                return false;
            }
            bind(vi, place);
            bind_name(vi->name_view(), place);
            if (async_ && vi->value && vi->value->kind() == ValueKind::AwaitExpr) {
                auto it = async_site_index_.find(vi);
                if (it != async_site_index_.end()) {
                    async_result_places_[it->second] = place;
                    return async_lower_await_var_init(vi, it->second, error);
                }
                // no site: a transparent (non-suspending) await. Fall through to
                // the normal lowering, which handles `AwaitExpr` as its operand.
            }
            if (vi->known_type() || (vi->value && vi->value->getType())) {
                BaseType* vtype = vi->known_type();
                if (!vtype && vi->value) vtype = vi->value->getType();
                const MIRTypeRecord& vr = module_.types.get(vt);
                if (!(vr.flags & TF_TYPEDEF) && abi::is_aggregate(module_, vt)) {
                    register_destructible(place, vtype);
                }
            }
            if (vi->value) {
                // moving a destructible value out of another local/param
                {
                    Value* iv = vi->value;
                    if (iv->val_kind() == ValueKind::AccessChain) {
                        auto* c = iv->as_access_chain_unsafe();
                        if (c->values.size() == 1) iv = c->values[0];
                    }
                    if (iv->val_kind() == ValueKind::Identifier) {
                        auto* iid = iv->as_identifier_unsafe();
                        PlaceId src = place_for_linked(iid->linked);
                        if (src == MIR_INVALID_ID) src = place_for_name(iid->value);
                        if (src != MIR_INVALID_ID) mark_moved(src);
                    }
                }
                // convert to the declared type first (implicit constructors, e.g.
                // `var s : std::string = \`...\``)
                MIRExprResult r = vi->known_type()
                                      ? lower_arg_converted(vi->value, vi->known_type(), error)
                                      : lower_expr(vi->value, error);
                if (!r.ok()) return false;
                if (r.kind == MIRExprKind::Place) {
                    // aggregate initializer: move the temporary into the variable
                    builder_->move_init(place, static_cast<PlaceId>(r.id), vt);
                    return true;
                }
                if (r.kind == MIRExprKind::Address) {
                    // copy the pointee into the variable (e.g. `var d = *c`)
                    builder_->copy_ptr_to_place(place, static_cast<ValueId>(r.id));
                    return true;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "invalid initializer expression";
                    return false;
                }
                if (vt != MIR_INVALID_ID && vt < module_.types.size() &&
                    module_.types.get(vt).kind == MIRTypeKind::Array) {
                    // initialize an array from a pointer (e.g. a string literal):
                    // copy only the literal's bytes, leaving the rest zeroed
                    if (vi->value->val_kind() == ValueKind::String) {
                        auto* sv = vi->value->as_string_unsafe();
                        uint64_t n = static_cast<uint64_t>(sv->value.size()) + 1; // include NUL
                        const uint64_t arr_len = module_.types.get(vt).data_count;
                        if (arr_len != 0 && n > arr_len) n = arr_len;
                        // The literal only covers `n` bytes, so zero the tail of
                        // the destination explicitly (the alloca is no longer
                        // unconditionally zeroed; see evaluation §1.2 / action C1).
                        builder_->zero_init(place);
                        const TypeId ut = types_.u32_type();
                        const ValueId sz = builder_->const_int(
                            ut, module_.constants.add_int(ut, n));
                        builder_->memcpy_size(place, static_cast<ValueId>(r.id), sz);
                    } else {
                        builder_->copy_ptr_to_place(place, static_cast<ValueId>(r.id));
                    }
                    return true;
                }
                ValueId val = r.id;
                if (r.type != vt) val = builder_->cast(val, vt);
                builder_->store(place, val);
            }
            return true;
        }
        case ASTNodeKind::AssignmentStmt: {
            auto* as = node->as_assignment();
            if (!as->lhs) {
                error = "assignment has no target";
                return false;
            }
            Value* lhs = as->lhs;
            // an unqualified field assignment inside a method resolves to
            // `self.field`; synthesize the member chain `self.field`
            std::unique_ptr<VariableIdentifier> implicit_self;
            std::unique_ptr<AccessChain> implicit_chain;
            if (lhs->val_kind() == ValueKind::Identifier && current_self_param_) {
                auto* lid = lhs->as_identifier_unsafe();
                if (is_implicit_field_node(lid->linked)) {
                    implicit_self = std::make_unique<VariableIdentifier>(
                        current_self_param_->name, current_self_param_->type, ZERO_LOC);
                    implicit_self->linked = current_self_param_;
                    std::vector<Value*> vals = {implicit_self.get(), lhs};
                    implicit_chain = std::make_unique<AccessChain>(
                        std::move(vals), lhs->getType(), ZERO_LOC);
                    lhs = implicit_chain.get();
                }
            }
            if (lhs->val_kind() == ValueKind::AccessChain) {
                auto* chain = lhs->as_access_chain_unsafe();
                if (chain->values.size() == 1) {
                    lhs = chain->values[0];
                } else if (chain->values.size() >= 2) {
                    const size_t n = chain->values.size();
                    // walk to the base of the last field
                    MIRExprResult first = lower_expr(chain->values[0], error);
                    if (!first.ok()) return false;
                    PlaceId cur_place = MIR_INVALID_ID;
                    ValueId cur_ptr = MIR_NULL;
                    bool have_ptr = false;
                    if (first.kind == MIRExprKind::Place) {
                        cur_place = static_cast<PlaceId>(first.id);
                    } else if (first.kind == MIRExprKind::Value ||
                               first.kind == MIRExprKind::Address) {
                        cur_ptr = static_cast<ValueId>(first.id);
                        have_ptr = true;
                    } else {
                        error = "field assignment base is not a place or pointer";
                        return false;
                    }
                    for (size_t i = 1; i + 1 < n; ++i) {
                        std::string fname;
                        if (!member_field_path(chain->values[i - 1], chain->values[i], fname, error)) return false;
                        const ConstantId fc = module_.constants.add_string(
                            MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                        const TypeId ft = types_.map(chain->values[i]->getType());
                        const MIRTypeRecord& ftr = module_.types.get(ft);
                        const ValueId faddr = have_ptr
                                                  ? builder_->field_addr_ptr(cur_ptr, fc, ft)
                                                  : builder_->field_addr(cur_place, fc, ft);
                        // an intermediate pointer/reference field is dereferenced
                        // so the next field is reached through it (`a.b->c`)
                        cur_ptr = (ftr.kind == MIRTypeKind::Pointer ||
                                   ftr.kind == MIRTypeKind::Reference)
                                      ? builder_->load_indirect(faddr, ft)
                                      : faddr;
                        have_ptr = true;
                    }
                    std::string fname;
                    if (!member_field_path(chain->values[n - 2], chain->values.back(), fname, error)) return false;
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, fname.data(), static_cast<uint32_t>(fname.size()));
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    if (r.kind == MIRExprKind::Value &&
                        as->assOp != Operation::Assignment) {
                        // compound assignment on a struct field: load, apply, store
                        MIRBinaryOp bop = MIRBinaryOp::Add;
                        if (!compound_binary(as->assOp, bop)) {
                            error = "unsupported compound field assignment operator";
                            return false;
                        }
                        const TypeId ft = types_.map(chain->values.back()->getType());
                        const ValueId cur = have_ptr ? builder_->field_load_ptr(cur_ptr, fc, ft)
                                                     : builder_->field_load(cur_place, fc, ft);
                        const ConstantId opc = module_.constants.add_int(
                            MIR_INVALID_ID, static_cast<uint64_t>(bop));
                        if (abi::is_struct_like(module_, ft)) {
                            error = "compound field assignment operator overloading is not yet supported";
                            return false;
                        }
                        const ValueId nv = builder_->binary(cur, r.id, opc, ft);
                        if (!have_ptr) {
                            builder_->field_store(cur_place, fc, nv);
                        } else {
                            builder_->field_store_ptr(cur_ptr, fc, nv);
                        }
                        return true;
                    }
                    if (as->assOp == Operation::Assignment) {
                        // a simple assignment destroys the previous value of a
                        // destructible field before overwriting it
                        BaseType* fbt = chain->values.back()->getType();
                        const SymbolId fdtor = destructor_symbol(fbt);
                        if (fdtor != MIR_INVALID_ID) {
                            const TypeId ft = types_.map(fbt);
                            const ValueId faddr =
                                have_ptr ? builder_->field_addr_ptr(cur_ptr, fc, ft)
                                         : builder_->field_addr(cur_place, fc, ft);
                            const TypeId ptrt = types_.pointer_type(ft, true);
                            MIROperand arg = MIROperand::value(faddr, ptrt);
                            builder_->call_scalar(fdtor, types_.void_type(), &arg, 1);
                        }
                    }
                    if (r.kind == MIRExprKind::Place) {
                        // a move from a local clears the source's drop flag so it
                        // isn't destroyed again after being moved into the field
                        {
                            Value* rv = as->value;
                            if (rv && rv->val_kind() == ValueKind::AccessChain) {
                                auto* c = rv->as_access_chain_unsafe();
                                if (c->values.size() == 1) rv = c->values[0];
                            }
                            if (rv && rv->val_kind() == ValueKind::Identifier) {
                                auto* rid = rv->as_identifier_unsafe();
                                PlaceId src = place_for_linked(rid->linked);
                                if (src == MIR_INVALID_ID) src = place_for_name(rid->value);
                                if (src != MIR_INVALID_ID) mark_moved(src);
                            }
                        }
                        if (!have_ptr) {
                            builder_->field_store_place(cur_place, fc,
                                                        static_cast<PlaceId>(r.id));
                        } else {
                            const TypeId vt = builder_->function().places[r.id].type;
                            const ValueId sptr = builder_->address_of(
                                static_cast<PlaceId>(r.id), types_.pointer_type(vt, false));
                            const ValueId sv = builder_->load_indirect(sptr, vt);
                            builder_->field_store_ptr(cur_ptr, fc, sv);
                        }
                        return true;
                    }
                    if (r.kind == MIRExprKind::Address) {
                        const TypeId ft2 = types_.map(chain->values.back()->getType());
                        const ValueId loaded =
                            builder_->load_indirect(static_cast<ValueId>(r.id), ft2);
                        if (!have_ptr) {
                            builder_->field_store(cur_place, fc, loaded);
                        } else {
                            builder_->field_store_ptr(cur_ptr, fc, loaded);
                        }
                        return true;
                    }
                    if (r.kind != MIRExprKind::Value) {
                        error = "aggregate field assignment not yet supported";
                        return false;
                    }
                    if (!have_ptr) {
                        builder_->field_store(cur_place, fc, r.id);
                    } else {
                        builder_->field_store_ptr(cur_ptr, fc, r.id);
                    }
                    return true;
                }
            }
            if (lhs->val_kind() == ValueKind::DereferenceValue) {
                auto* d = lhs->as_dereference_value_unsafe();
                MIRExprResult p = lower_expr(d->getValue(), error);
                if (!p.ok()) return false;
                MIRExprResult r = lower_expr(as->value, error);
                if (!r.ok()) return false;
                const TypeId et = types_.map(lhs->getType());
                if (r.kind == MIRExprKind::Place) {
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::value(static_cast<ValueId>(p.id), p.type),
                                    MIROperand::place(static_cast<PlaceId>(r.id), et)});
                    return true;
                }
                if (r.kind == MIRExprKind::Address) {
                    const ValueId loaded =
                        builder_->load_indirect(static_cast<ValueId>(r.id), et);
                    builder_->store_indirect(static_cast<ValueId>(p.id), loaded);
                    return true;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "aggregate dereference assignment not yet supported";
                    return false;
                }
                if (as->assOp != Operation::Assignment) {
                    MIRBinaryOp bop = MIRBinaryOp::Add;
                    if (!compound_binary(as->assOp, bop)) {
                        error = "unsupported compound dereference assignment operator";
                        return false;
                    }
                    const ConstantId opc = module_.constants.add_int(
                        MIR_INVALID_ID, static_cast<uint64_t>(bop));
                    const ValueId cur =
                        builder_->load_indirect(static_cast<ValueId>(p.id), et);
                    if (abi::is_struct_like(module_, et)) {
                        error = "compound dereference assignment operator overloading is not yet supported";
                        return false;
                    }
                    const ValueId nv = builder_->binary(cur, r.id, opc, et);
                    builder_->store_indirect(static_cast<ValueId>(p.id), nv);
                } else {
                    builder_->store_indirect(static_cast<ValueId>(p.id), r.id);
                }
                return true;
            }
            if (lhs->val_kind() == ValueKind::IndexOperator) {
                auto* io = lhs->as_index_op_unsafe();
                MIRExprResult base = lower_expr(io->parent_val, error);
                if (!base.ok()) return false;
                maybe_register_call_temp(io->parent_val, base);
                MIRExprResult idx = lower_expr(io->idx, error);
                if (!idx.ok()) return false;
                if (base.kind == MIRExprKind::Place) {
                    const TypeId bt = builder_->function().places[base.id].type;
                    if (abi::is_struct_like(module_, bt)) {
                        // `x[i] = v` on a struct uses the `IndexMut` method
                        BaseType* recv_t = io->parent_val ? io->parent_val->getType() : nullptr;
                        FunctionDeclaration* fd = resolve_method_fallback(recv_t, "index_mut");
                        if (!fd) fd = resolve_method_fallback(recv_t, "index");
                        if (!fd) {
                            error = "index operator overloading is not yet supported";
                            return false;
                        }
                        VariableIdentifier op_id(chem::string_view("index"), nullptr, ZERO_LOC);
                        op_id.linked = fd;
                        FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
                        op_call.values.push_back(io->idx);
                        MIRExprResult ref = lower_method_call(base, recv_t, &op_call, error);
                        if (!ref.ok()) return false;
                        if (ref.kind != MIRExprKind::Value && ref.kind != MIRExprKind::Address) {
                            error = "index operator did not return an address";
                            return false;
                        }
                        MIRExprResult r = lower_expr(as->value, error);
                        if (!r.ok()) return false;
                        const TypeId et = types_.map(io->getType());
                        ValueId rv = MIR_NULL;
                        if (r.kind == MIRExprKind::Place) {
                            rv = builder_->load(static_cast<PlaceId>(r.id), et);
                        } else if (r.kind == MIRExprKind::Address) {
                            rv = builder_->load_indirect(static_cast<ValueId>(r.id), et);
                        } else if (r.kind == MIRExprKind::Value) {
                            rv = static_cast<ValueId>(r.id);
                        } else {
                            error = "index assignment value is not a value";
                            return false;
                        }
                        builder_->store_indirect(static_cast<ValueId>(ref.id), rv);
                        return true;
                    }
                }
                MIRExprResult r = lower_expr(as->value, error);
                if (!r.ok()) return false;
                const TypeId et = types_.map(io->getType());
                if (r.kind == MIRExprKind::Place) {
                    if (base.kind == MIRExprKind::Place) {
                        builder_->element_store_place(static_cast<PlaceId>(base.id),
                                                      static_cast<ValueId>(idx.id),
                                                      static_cast<PlaceId>(r.id));
                    } else {
                        const ValueId eaddr = builder_->index_addr_ptr(
                            static_cast<ValueId>(base.id),
                            static_cast<ValueId>(idx.id), et);
                        builder_->emit(MIROpcode::Store, MIR_NULL,
                                       {MIROperand::value(eaddr, et),
                                        MIROperand::place(static_cast<PlaceId>(r.id), et)});
                    }
                    return true;
                }
                if (r.kind == MIRExprKind::Address) {
                    const ValueId loaded =
                        builder_->load_indirect(static_cast<ValueId>(r.id), et);
                    if (base.kind == MIRExprKind::Place) {
                        builder_->element_store(static_cast<PlaceId>(base.id),
                                                static_cast<ValueId>(idx.id), loaded);
                    } else {
                        builder_->index_store(static_cast<ValueId>(base.id),
                                              static_cast<ValueId>(idx.id), loaded);
                    }
                    return true;
                }
                if (r.kind != MIRExprKind::Value) {
                    error = "aggregate index assignment not yet supported";
                    return false;
                }
                const bool compound = (as->assOp != Operation::Assignment);
                if (compound && abi::is_struct_like(module_, et)) {
                    error = "compound index assignment operator overloading is not yet supported";
                    return false;
                }
                ConstantId opc = MIR_INVALID_ID;
                if (compound) {
                    MIRBinaryOp bop = MIRBinaryOp::Add;
                    if (!compound_binary(as->assOp, bop)) {
                        error = "unsupported compound index assignment operator";
                        return false;
                    }
                    opc = module_.constants.add_int(MIR_INVALID_ID,
                                                   static_cast<uint64_t>(bop));
                }
                if (base.kind == MIRExprKind::Place) {
                    const PlaceId bp = static_cast<PlaceId>(base.id);
                    ValueId val = r.id;
                    if (compound) {
                        const ValueId cur =
                            builder_->element_load(bp, static_cast<ValueId>(idx.id), et);
                        val = builder_->binary(cur, r.id, opc, et);
                    }
                    builder_->element_store(bp, static_cast<ValueId>(idx.id), val);
                } else {
                    const ValueId bv = static_cast<ValueId>(base.id);
                    ValueId val = r.id;
                    if (compound) {
                        const ValueId cur =
                            builder_->index_load(bv, static_cast<ValueId>(idx.id), et);
                        val = builder_->binary(cur, r.id, opc, et);
                    }
                    builder_->index_store(bv, static_cast<ValueId>(idx.id), val);
                }
                return true;
            }
            if (lhs->val_kind() != ValueKind::Identifier) {
                error = "assignment target is not a simple local variable yet";
                return false;
            }
            auto* id = lhs->as_identifier_unsafe();
            PlaceId place = place_for_linked(id->linked);
            if (place == MIR_INVALID_ID && id->linked &&
                (id->linked->kind() == ASTNodeKind::PatternMatchId ||
                 id->linked->kind() == ASTNodeKind::VariantCaseVariable)) {
                // assignment to a destructured variant payload field
                MIRExprResult addr = lower_address_of(lhs, error);
                if (!addr.ok()) return false;
                if (addr.kind != MIRExprKind::Value && addr.kind != MIRExprKind::Address) {
                    error = "variant payload assignment target is not addressable";
                    return false;
                }
                MIRExprResult r = lower_expr(as->value, error);
                if (!r.ok()) return false;
                const TypeId et = types_.map(id->getType());
                ValueId rv = MIR_NULL;
                if (r.kind == MIRExprKind::Place) {
                    rv = builder_->load(static_cast<PlaceId>(r.id), et);
                } else if (r.kind == MIRExprKind::Address) {
                    rv = builder_->load_indirect(static_cast<ValueId>(r.id), et);
                } else if (r.kind == MIRExprKind::Value) {
                    rv = static_cast<ValueId>(r.id);
                } else {
                    error = "variant payload assignment value is not a value";
                    return false;
                }
                builder_->store_indirect(static_cast<ValueId>(addr.id), rv);
                return true;
            }
            if (place == MIR_INVALID_ID) {
                // store the RHS into a global, applying a compound operator
                // (`g += x`) by loading the current value first
                auto store_global = [&](ValueId addr, TypeId vt, MIRExprResult r) -> bool {
                    ValueId rv = MIR_NULL;
                    bool have = false;
                    if (r.kind == MIRExprKind::Place) {
                        rv = builder_->load(static_cast<PlaceId>(r.id), vt);
                        have = true;
                    } else if (r.kind == MIRExprKind::Address) {
                        const MIRTypeRecord& gtr = module_.types.get(vt);
                        if (gtr.kind == MIRTypeKind::Pointer ||
                            gtr.kind == MIRTypeKind::Reference) {
                            rv = static_cast<ValueId>(r.id);
                        } else {
                            rv = builder_->load_indirect(static_cast<ValueId>(r.id), vt);
                        }
                        have = true;
                    } else if (r.kind == MIRExprKind::Value) {
                        rv = static_cast<ValueId>(r.id);
                        have = true;
                    }
                    if (!have) {
                        error = "aggregate global assignment not yet supported";
                        return false;
                    }
                    if (as->assOp != Operation::Assignment) {
                        MIRBinaryOp bop = MIRBinaryOp::Add;
                        if (!compound_binary(as->assOp, bop)) {
                            error = "unsupported compound global assignment operator";
                            return false;
                        }
                        const ConstantId opc = module_.constants.add_int(
                            MIR_INVALID_ID, static_cast<uint64_t>(bop));
                        const ValueId cur = builder_->load_indirect(addr, vt);
                        const ValueId nv = builder_->binary(cur, rv, opc, vt);
                        builder_->store_indirect(addr, nv);
                        return true;
                    }
                    builder_->store_indirect(addr, rv);
                    return true;
                };
                if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
                    auto* vi = id->linked->as_var_init();
                    const SymbolId gs = intern_global(vi);
                    const TypeId vt = types_.map(id->getType());
                    const TypeId ptrt = types_.pointer_type(vt, true);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    return store_global(addr, vt, r);
                }
                const std::string gname(id->value.data(), id->value.size());
                if (!gname.empty()) {
                    const SymbolId gs = intern_named_global(gname);
                    const TypeId vt = types_.map(id->getType());
                    const TypeId ptrt = types_.pointer_type(vt, true);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    return store_global(addr, vt, r);
                }
                error = "assignment target does not resolve to a local place";
                return false;
            }
            const TypeId place_type = builder_->function().places[place].type;
            if (as->assOp != Operation::Assignment && abi::is_struct_like(module_, place_type)) {
                // `p += s` on a struct uses an overloaded `add_assign`-style method
                const char* method = compound_method_name(as->assOp);
                BaseType* lhs_ast = as->lhs ? as->lhs->getType() : nullptr;
                FunctionDeclaration* fd =
                    method ? resolve_method_fallback(lhs_ast, method) : nullptr;
                if (!fd) {
                    error = "compound assignment operator overloading is not yet supported";
                    return false;
                }
                VariableIdentifier op_id(
                    chem::string_view(method, std::strlen(method)), nullptr, ZERO_LOC);
                op_id.linked = fd;
                FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
                op_call.values.push_back(as->value);
                MIRExprResult recv = MIRExprResult::place(place, place_type);
                MIRExprResult rr = lower_method_call(recv, lhs_ast, &op_call, error);
                return rr.ok();
            }
            // a move from a local clears the source's drop flag
            {
                Value* rv = as->value;
                if (rv && rv->val_kind() == ValueKind::AccessChain) {
                    auto* c = rv->as_access_chain_unsafe();
                    if (c->values.size() == 1) rv = c->values[0];
                }
                if (rv && rv->val_kind() == ValueKind::Identifier) {
                    auto* rid = rv->as_identifier_unsafe();
                    PlaceId src = place_for_linked(rid->linked);
                    if (src == MIR_INVALID_ID) src = place_for_name(rid->value);
                    if (src != MIR_INVALID_ID) mark_moved(src);
                }
            }
            // convert to the target's declared type first (implicit constructors,
            // e.g. `s = \`...\``)
            MIRExprResult r = as->lhs->getType()
                                  ? lower_arg_converted(as->value, as->lhs->getType(), error)
                                  : lower_expr(as->value, error);
            if (!r.ok()) return false;
            auto destroy_old_dest = [&]() {
                // a simple assignment destroys the previous value of a
                // destructible local before overwriting it
                if (as->assOp != Operation::Assignment || as->is_first_init) return;
                for (auto& d : destructibles_) {
                    if (d.place == place) {
                        const ValueId fv = builder_->load(d.flag, types_.bool_type());
                        builder_->drop(d.place, d.dtor, fv);
                        break;
                    }
                }
            };
            auto mark_initialized = [&]() {
                // the place now holds a live value again (e.g. after a move or a
                // re-initializing assignment), so re-arm its drop flag
                for (auto& d : destructibles_) {
                    if (d.place == place) {
                        builder_->set_drop(d.flag, true);
                        break;
                    }
                }
            };
            if (r.kind == MIRExprKind::Place) {
                destroy_old_dest();
                builder_->emit(MIROpcode::Store, MIR_NULL,
                               {MIROperand::place(place, place_type),
                                MIROperand::place(static_cast<PlaceId>(r.id), place_type)});
                mark_initialized();
                return true;
            }
            if (r.kind == MIRExprKind::Address) {
                destroy_old_dest();
                const MIRTypeRecord& ptr = module_.types.get(place_type);
                if (ptr.kind == MIRTypeKind::Pointer || ptr.kind == MIRTypeKind::Reference) {
                    // the target is a pointer/reference: the address is the value
                    builder_->store(place, static_cast<ValueId>(r.id));
                } else if (ptr.kind == MIRTypeKind::Array) {
                    // arrays are not assignable in C: the emitter copies the
                    // source bytes from the address (a load would declare an
                    // array initialized from a dereference, which is invalid C)
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::place(place, place_type),
                                    MIROperand::value(static_cast<ValueId>(r.id), r.type)});
                } else {
                    const ValueId loaded =
                        builder_->load_indirect(static_cast<ValueId>(r.id), place_type);
                    builder_->store(place, loaded);
                }
                mark_initialized();
                return true;
            }
            if (r.kind != MIRExprKind::Value) {
                error = "aggregate assignment not yet supported";
                return false;
            }
            ValueId val = r.id;
            if (as->assOp != Operation::Assignment) {
                MIROpcode cop = MIROpcode::Nop;
                MIRBinaryOp bop = MIRBinaryOp::Add;
                if (!(map_binary(as->assOp, cop, bop) && cop == MIROpcode::Binary) &&
                    !compound_binary(as->assOp, bop)) {
                    error = "unsupported compound assignment operator (" +
                            std::to_string(static_cast<int>(as->assOp)) + ")";
                    return false;
                }
                const ValueId cur = builder_->load(place, place_type);
                const ConstantId opc = module_.constants.add_int(
                    MIR_INVALID_ID, static_cast<uint64_t>(bop));
                val = builder_->binary(cur, val, opc, place_type);
            } else if (r.type != place_type) {
                val = builder_->cast(val, place_type);
            }
            builder_->store(place, val);
            mark_initialized();
            return true;
        }
        case ASTNodeKind::ReturnStmt: {
            auto* rs = node->as_return();
            if (async_) {
                const TypeId inner_t = types_.map(async_inner_);
                const ConstantId res_fc =
                    module_.constants.add_string(MIR_INVALID_ID, "__result", 8);
                if (async_in_ramp_) {
                    if (rs->value) {
                        MIRExprResult r = lower_expr(rs->value, error);
                        if (!r.ok()) return false;
                        ValueId v = (r.kind == MIRExprKind::Place)
                                        ? builder_->load(static_cast<PlaceId>(r.id), inner_t)
                                        : static_cast<ValueId>(r.id);
                        builder_->field_store_ptr(async_ramp_af_val_, res_fc, v);
                    }
                    emit_drops();
                    builder_->br(async_ramp_done_);
                    return true;
                }
                if (rs->value) {
                    MIRExprResult r = lower_expr(rs->value, error);
                    if (!r.ok()) return false;
                    ValueId v = (r.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(r.id), inner_t)
                                    : static_cast<ValueId>(r.id);
                    builder_->field_store_ptr(async_poll_af_val_, res_fc, v);
                }
                emit_drops();
                builder_->emit(
                    MIROpcode::AsyncFinish, MIR_NULL,
                    {MIROperand::place(async_poll_frame_, async_frame_ptr_type_),
                     MIROperand::place(async_poll_ret_,
                                       types_.pointer_type(types_.map(async_poll_base_), true)),
                     MIROperand::type(types_.map(async_poll_base_))});
                return true;
            }
            // returning a local destructible moves it out (do not drop it here)
            if (rs->value) {
                Value* rv = rs->value;
                // unwrap `unsafe(...)`, a 1-element chain, etc. to find the moved local
                for (int guard = 0; guard < 4 && rv; ++guard) {
                    if (rv->val_kind() == ValueKind::AccessChain) {
                        auto* c = rv->as_access_chain_unsafe();
                        if (c->values.size() == 1) { rv = c->values[0]; continue; }
                        break;
                    }
                    if (rv->val_kind() == ValueKind::UnsafeValue) {
                        auto* u = static_cast<UnsafeValue*>(rv);
                        if (u->getValue()) { rv = u->getValue(); continue; }
                        break;
                    }
                    if (rv->val_kind() == ValueKind::FunctionCall) {
                        auto* fc = rv->as_func_call_unsafe();
                        std::string cn;
                        if (fc->parent_val && fc->parent_val->val_kind() == ValueKind::Identifier) {
                            auto* cid = fc->parent_val->as_identifier_unsafe();
                            cn.assign(cid->value.data(), cid->value.size());
                        }
                        if (cn == "unsafe" && fc->values.size() == 1) { rv = fc->values[0]; continue; }
                        break;
                    }
                    break;
                }
                if (rv->val_kind() == ValueKind::Identifier) {
                    auto* id = rv->as_identifier_unsafe();
                    PlaceId rp = place_for_linked(id->linked);
                    if (rp == MIR_INVALID_ID) rp = place_for_name(id->value);
                    if (rp != MIR_INVALID_ID) mark_moved(rp);
                }
            }
            if (!rs->value) {
                if (destructor_def_) emit_destructor_members(error);
                emit_drops();
                builder_->ret_void();
                return true;
            }
            MIRExprResult r;
            if (sret_) {
                // convert to the declared return type first (implicit constructors)
                r = lower_arg_converted(rs->value, return_type_, error);
            } else {
                r = lower_expr(rs->value, error);
            }
            if (!r.ok()) return false;
            emit_drops();
            if (r.kind == MIRExprKind::Void) {
                // `return void_call()` in a void function
                if (destructor_def_) emit_destructor_members(error);
                builder_->ret_void();
                return true;
            }
            if (sret_) {
                if (r.kind == MIRExprKind::Place) {
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::value(sret_ptr_, sret_ptr_type_),
                                    MIROperand::place(static_cast<PlaceId>(r.id), sret_ret_type_)});
                } else if (r.kind == MIRExprKind::Address) {
                    // `return &raw obj.field` / an aggregate address: copy the
                    // pointee into the sret destination
                    const ValueId loaded =
                        builder_->load_indirect(static_cast<ValueId>(r.id), sret_ret_type_);
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::value(sret_ptr_, sret_ptr_type_),
                                    MIROperand::value(loaded, sret_ret_type_)});
                } else if (r.kind == MIRExprKind::Value) {
                    builder_->emit(MIROpcode::Store, MIR_NULL,
                                   {MIROperand::value(sret_ptr_, sret_ptr_type_),
                                    MIROperand::value(static_cast<ValueId>(r.id), sret_ret_type_)});
                } else {
                    error = "returning an aggregate value is not yet supported";
                    return false;
                }
                builder_->ret_void();
                return true;
            }
            if (r.kind == MIRExprKind::Place) {
                // a by-value aggregate (typedef struct, e.g. a fat pointer) return
                const TypeId pt = builder_->function().places[r.id].type;
                builder_->ret(builder_->load(static_cast<PlaceId>(r.id), pt));
                return true;
            }
            if (r.kind != MIRExprKind::Value && r.kind != MIRExprKind::Address) {
                error = "returning a non-value expression is not yet supported";
                return false;
            }
            builder_->ret(r.id);
            return true;
        }
        case ASTNodeKind::EmbeddedNode: {
            auto* en = node->as_embedded_node_unsafe();
            ASTNode* repl = en->replacement;
            if (!repl && embedded_repl_) repl = embedded_repl_(en);
            if (repl) {
                en->replacement = repl;
                if (repl->kind() == ASTNodeKind::Scope) {
                    return lower_scope(*repl->as_scope_unsafe(), error);
                }
                return lower_stmt(repl, error);
            }
            error = "embedded node has no replacement";
            return false;
        }
        case ASTNodeKind::ValueWrapper: {
            auto* vw = node->as_value_wrapper();
            MIRExprResult r = lower_expr(vw->value, error);
            if (!r.ok()) return false;
            // a bare struct-returning call statement owns a temporary
            destroy_call_temp(vw->value, r);
            return true;
        }
        case ASTNodeKind::ValueNode: {
            auto* vn = node->as_value_node_unsafe();
            MIRExprResult r = lower_expr(vn->value, error);
            if (!r.ok()) return false;
            destroy_call_temp(vn->value, r);
            return true;
        }
        case ASTNodeKind::IfStmt: {
            auto* ifs = node->as_if_stmt_unsafe();
            // a condition the resolver proved constant (a `comptime if`, or a
            // condition such as `def.is64Bit`) carries its taken scope here
            if (ifs->computed_scope.has_value()) {
                Scope* taken = ifs->computed_scope.value();
                if (taken) return lower_scope(*taken, error);
                return true;
            }
            if (ifs->is_comptime()) {
                // compile-time condition: lower only the taken scope
                std::string rerr;
                Scope* taken = comptime_if_resolver_ ? comptime_if_resolver_(ifs, rerr) : nullptr;
                if (taken) return lower_scope(*taken, error);
                // if the condition cannot be resolved at comptime, fall back to
                // lowering it as a runtime branch (matches the legacy backend)
            }
            const BlockId merge = builder_->create_block();
            if (!lower_if(ifs, merge, error)) return false;
            builder_->set_block(merge);
            return true;
        }
        case ASTNodeKind::WhileLoopStmt:
            return lower_while(node->as_while_loop_unsafe(), error);
        case ASTNodeKind::DoWhileLoopStmt:
            return lower_do_while(node->as_do_while_loop_unsafe(), error);
        case ASTNodeKind::LoopBlock:
            return lower_loop_block(node->as_loop_block_unsafe(), error);
        case ASTNodeKind::ForLoopStmt:
            return lower_for(node->as_for_loop_unsafe(), error);
        case ASTNodeKind::ForInLoopStmt:
            return lower_for_in(node->as_for_in_loop_unsafe(), error);
        case ASTNodeKind::PlacementNewNode: {
            auto* pn = node->as_placement_new_node_unsafe();
            MIRExprResult r = lower_expr(&pn->value, error);
            return r.ok();
        }
        case ASTNodeKind::DeleteStmt: {
            // `delete ptr` / `destruct ptr` / `destruct[count] ptr`
            auto* ds = node->as_destruct_stmt_unsafe();
            BaseType* idt = ds->identifier ? ds->identifier->getType() : nullptr;
            BaseType* ct = idt ? idt->canonical() : nullptr;
            BaseType* elem_ast = nullptr;
            if (ct && ct->kind() == BaseTypeKind::Array) {
                elem_ast = ct->as_array_type_unsafe()->elem_type;
            } else if (ct && ct->kind() == BaseTypeKind::Pointer) {
                elem_ast = ct->as_pointer_type_unsafe()->type;
            }
            const SymbolId dtor = elem_ast ? destructor_symbol(elem_ast) : MIR_INVALID_ID;
            if (dtor == MIR_INVALID_ID && !ds->getFreeAfter()) return true;
            MIRExprResult p = lower_expr(ds->identifier, error);
            if (!p.ok()) return false;
            const ValueId ptr = (p.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(p.id), p.type)
                                    : static_cast<ValueId>(p.id);
            const TypeId vpt = types_.pointer_type(types_.void_type(), true);
            const TypeId void_t = types_.void_type();
            if (dtor != MIR_INVALID_ID) {
                const TypeId bool_t = types_.bool_type();
                const TypeId u64t = types_.u64_type();
                const ConstantId eq_op =
                    module_.constants.add_int(MIR_INVALID_ID,
                                              static_cast<uint64_t>(MIRBinaryOp::Eq));
                const ValueId null_v =
                    builder_->const_null(vpt, module_.constants.add_null(vpt));
                const ValueId is_null = builder_->compare(ptr, null_v, eq_op, bool_t);
                const BlockId nonnull_b = builder_->create_block();
                const BlockId done_b = builder_->create_block();
                builder_->cond_br(is_null, done_b, nonnull_b);
                builder_->set_block(nonnull_b);
                const TypeId eptr = types_.pointer_type(types_.map(elem_ast), true);
                if (ds->is_array) {
                    // `destruct[count] ptr`: destroy each element, last to first
                    const ConstantId add_op =
                        module_.constants.add_int(MIR_INVALID_ID,
                                                  static_cast<uint64_t>(MIRBinaryOp::Add));
                    const ConstantId sub_op =
                        module_.constants.add_int(MIR_INVALID_ID,
                                                  static_cast<uint64_t>(MIRBinaryOp::Sub));
                    const ConstantId gt_op =
                        module_.constants.add_int(MIR_INVALID_ID,
                                                  static_cast<uint64_t>(MIRBinaryOp::Gt));
                    ValueId count = builder_->const_int(
                        u64t, module_.constants.add_int(u64t, 0));
                    if (ds->array_value) {
                        MIRExprResult cv = lower_expr(ds->array_value, error);
                        if (!cv.ok()) return false;
                        count = (cv.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(cv.id), u64t)
                                    : static_cast<ValueId>(cv.id);
                    }
                    const PlaceId iv = builder_->alloca(u64t, MIRStorageClass::Local);
                    builder_->store(iv, count);
                    const BlockId head = builder_->create_block();
                    const BlockId body = builder_->create_block();
                    const BlockId loop_done = builder_->create_block();
                    builder_->br(head);
                    builder_->set_block(head);
                    const ValueId cur = builder_->load(iv, u64t);
                    const ValueId zero =
                        builder_->const_int(u64t, module_.constants.add_int(u64t, 0));
                    const ValueId gt = builder_->compare(cur, zero, gt_op, bool_t);
                    builder_->cond_br(gt, body, loop_done);
                    builder_->set_block(body);
                    const ValueId one =
                        builder_->const_int(u64t, module_.constants.add_int(u64t, 1));
                    const ValueId dec = builder_->binary(cur, one, sub_op, u64t);
                    builder_->store(iv, dec);
                    const ValueId ep = builder_->binary(ptr, dec, add_op, eptr);
                    MIROperand a = MIROperand::value(ep, eptr);
                    builder_->call_scalar(dtor, void_t, &a, 1);
                    builder_->br(head);
                    builder_->set_block(loop_done);
                } else {
                    MIROperand a = MIROperand::value(ptr, vpt);
                    builder_->call_scalar(dtor, void_t, &a, 1);
                }
                builder_->br(done_b);
                builder_->set_block(done_b);
            }
            if (ds->getFreeAfter()) emit_free(ptr);
            return true;
        }
        case ASTNodeKind::DeallocStmt: {
            auto* da = static_cast<DeallocStmt*>(node);
            MIRExprResult p = lower_expr(da->ptr, error);
            if (!p.ok()) return false;
            const ValueId ptr = (p.kind == MIRExprKind::Place)
                                    ? builder_->load(static_cast<PlaceId>(p.id), p.type)
                                    : static_cast<ValueId>(p.id);
            emit_free(ptr);
            return true;
        }
        case ASTNodeKind::BreakStmt: {
            if (break_targets_.empty()) {
                error = "break outside a loop";
                return false;
            }
            auto* bs = node->as_break_stmt_unsafe();
            if (bs->value && !break_value_dests_.empty() &&
                break_value_dests_.back() != MIR_INVALID_ID) {
                const PlaceId dest = break_value_dests_.back();
                const TypeId vt = break_value_types_.back();
                MIRExprResult r = lower_expr(bs->value, error);
                if (!r.ok()) return false;
                if (r.kind == MIRExprKind::Place) {
                    builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), vt));
                } else if (r.kind == MIRExprKind::Address) {
                    builder_->store(dest, builder_->load_indirect(static_cast<ValueId>(r.id), vt));
                } else if (r.kind == MIRExprKind::Value) {
                    builder_->store(dest, static_cast<ValueId>(r.id));
                } else {
                    error = "loop break value does not produce a value";
                    return false;
                }
            }
            if (!loop_destructible_marks_.empty()) {
                emit_drops_from(loop_destructible_marks_.back());
            }
            builder_->br(break_targets_.back());
            return true;
        }
        case ASTNodeKind::ContinueStmt: {
            if (continue_targets_.empty()) {
                error = "continue outside a loop";
                return false;
            }
            if (!loop_destructible_marks_.empty()) {
                emit_drops_from(loop_destructible_marks_.back());
            }
            builder_->br(continue_targets_.back());
            return true;
        }
        case ASTNodeKind::IncDecNode: {
            auto* n = node->as_inc_dec_node_unsafe();
            MIRExprResult r =
                lower_incdec_value(n->value.getValue(), n->value.increment, n->value.post, error);
            return r.ok();
        }
        case ASTNodeKind::AccessChainNode: {
            auto* acn = node->as_access_chain_node_unsafe();
            auto& vals = acn->chain.values;
            if (vals.size() == 2 && vals[1] && vals[1]->val_kind() == ValueKind::FunctionCall) {
                MIRExprResult recv = lower_receiver(vals, 1, error);
                if (!recv.ok()) return false;
                MIRExprResult r = lower_method_call(recv, vals[0]->getType(),
                                                    vals[1]->as_func_call_unsafe(), error);
                return r.ok();
            }
            MIRExprResult r = lower_expr(&acn->chain, error);
            if (!r.ok()) return false;
            // a bare call/access-chain expression statement owns a temporary
            if (acn->chain.values.size() == 1) {
                destroy_call_temp(acn->chain.values[0], r);
            }
            return true;
        }
        case ASTNodeKind::SwitchStmt: {
            auto* sw = node->as_switch_stmt_unsafe();
            const BlockId end_b = builder_->create_block();
            std::vector<BlockId> scope_blocks(sw->scopes.size());
            for (size_t i = 0; i < sw->scopes.size(); ++i) {
                scope_blocks[i] = builder_->create_block();
            }
            const BlockId default_b =
                (sw->defScopeInd >= 0) ? scope_blocks[sw->defScopeInd] : end_b;
            if (!lower_switch_dispatch(*sw, scope_blocks, default_b, error)) return false;

            for (size_t i = 0; i < sw->scopes.size(); ++i) {
                builder_->set_block(scope_blocks[i]);
                if (!lower_scope(sw->scopes[i], error)) return false;
                if (!builder_->current_block_terminated()) builder_->br(end_b);
            }
            builder_->set_block(end_b);
            return true;
        }
        case ASTNodeKind::PatternMatchExprNode:
            return lower_pattern_match_node(node->as_pattern_match_expr_node_unsafe(), error);
        case ASTNodeKind::Block: {
            auto* b = node->as_block_scope_unsafe();
            return lower_scope_nodes_scoped(b->nodes, node->encoded_location().encoded, error);
        }
        case ASTNodeKind::Scope: {
            auto* s = node->as_scope_unsafe();
            return lower_scope_nodes_scoped(s->nodes, s->encoded_location().encoded, error);
        }
        case ASTNodeKind::UnsafeBlock: {
            auto* u = node->as_unsafe_block_unsafe();
            return lower_scope_nodes_scoped(u->scope.nodes, node->encoded_location().encoded, error);
        }
        case ASTNodeKind::InlineAsmStmt: {
            auto* ia = node->as_inline_asm_unsafe();
            std::vector<MIROperand> ops;
            const ConstantId tmpl = module_.constants.add_string(
                MIR_INVALID_ID, ia->asm_template.data(),
                static_cast<uint32_t>(ia->asm_template.size()));
            ops.push_back(MIROperand::constant(tmpl, MIR_INVALID_ID));
            auto count_const = [&](size_t n) {
                const ConstantId c =
                    module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(n));
                return MIROperand::constant(c, MIR_INVALID_ID);
            };
            auto emit_section = [&](std::vector<AsmOperand>& list) -> bool {
                ops.push_back(count_const(list.size()));
                for (AsmOperand& op : list) {
                    const ConstantId cc = module_.constants.add_string(
                        MIR_INVALID_ID, op.constraint.data(),
                        static_cast<uint32_t>(op.constraint.size()));
                    ops.push_back(MIROperand::constant(cc, MIR_INVALID_ID));
                    MIROperand o;
                    if (!lower_asm_operand(op.expr, o, error)) return false;
                    ops.push_back(o);
                }
                return true;
            };
            if (!emit_section(ia->output_operands)) return false;
            if (!emit_section(ia->input_operands)) return false;
            ops.push_back(count_const(ia->clobbers.size()));
            for (chem::string_view& cl : ia->clobbers) {
                const ConstantId cc = module_.constants.add_string(
                    MIR_INVALID_ID, cl.data(), static_cast<uint32_t>(cl.size()));
                ops.push_back(MIROperand::constant(cc, MIR_INVALID_ID));
            }
            builder_->emit(MIROpcode::InlineAsm, MIR_NULL, ops.data(),
                           static_cast<uint32_t>(ops.size()));
            return true;
        }
        case ASTNodeKind::TypealiasStmt:
            // a local `type` alias has no runtime effect
            return true;
        case ASTNodeKind::ComptimeBlock:
            // the block is evaluated by the interpreter; it has no runtime effect
            if (comptime_block_eval_) comptime_block_eval_(node);
            return true;
        default:
            error = "unsupported statement kind during MIR lowering (kind " +
                    std::to_string(static_cast<int>(node->kind())) + ")";
            return false;
    }
}

MIRExprResult MIRLowerer::lower_incdec_value(Value* target, bool increment, bool post,
                                             std::string& error) {
    Value* t = target;
    if (t && t->val_kind() == ValueKind::AccessChain) {
        auto* c = t->as_access_chain_unsafe();
        if (c->values.size() == 1) t = c->values[0];
    }
    if (!t) {
        error = "increment/decrement target is null";
        return MIRExprResult::error();
    }
    const TypeId vt = types_.map(t->getType());
    if (abi::is_struct_like(module_, vt)) {
        // `p++` / `--p` on a struct uses the `Increment`/`Decrement` methods
        const char* method = increment ? (post ? "inc_post" : "inc_pre")
                                       : (post ? "dec_post" : "dec_pre");
        BaseType* ast_t = t->getType();
        FunctionDeclaration* fd = resolve_method_fallback(ast_t, method);
        if (!fd) {
            error = "increment/decrement operator overloading is not yet supported";
            return MIRExprResult::error();
        }
        MIRExprResult recv = lower_receiver(std::vector<Value*>{t}, 1, error);
        if (!recv.ok()) return recv;
        VariableIdentifier op_id(chem::string_view(method, std::strlen(method)), nullptr,
                                 ZERO_LOC);
        op_id.linked = fd;
        FunctionCall op_call(&op_id, fd->returnType, ZERO_LOC);
        return lower_method_call(recv, ast_t, &op_call, error);
    }
    const ConstantId opc = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(increment ? MIRBinaryOp::Add : MIRBinaryOp::Sub));
    const TypeId it = types_.u32_type();
    const ValueId one = builder_->const_int(it, module_.constants.add_int(it, 1));

    // a simple local / parameter
    if (t->val_kind() == ValueKind::Identifier) {
        auto* id = t->as_identifier_unsafe();
        PlaceId place = place_for_linked(id->linked);
        if (place == MIR_INVALID_ID) place = place_for_name(id->value);
        if (place != MIR_INVALID_ID) {
            const MIRTypeRecord& pr = module_.types.get(vt);
            if (pr.kind == MIRTypeKind::Reference) {
                // `ref++` on a reference increments the referent
                const ValueId ptr = builder_->load(place, vt);
                const TypeId et = pr.element;
                const ValueId cur = builder_->load_indirect(ptr, et);
                const ValueId nv = builder_->binary(cur, one, opc, et);
                builder_->store_indirect(ptr, nv);
                return MIRExprResult::value(post ? cur : nv, vt);
            }
            const ValueId cur = builder_->load(place, vt);
            const ValueId nv = builder_->binary(cur, one, opc, vt);
            builder_->store(place, nv);
            // a post-increment/decrement expression yields the *old* value
            return MIRExprResult::value(post ? cur : nv, vt);
        }
    }

    // any other lvalue (global, field, element, `*ptr`): go through its address
    MIRExprResult addr;
    if (t->val_kind() == ValueKind::DereferenceValue) {
        addr = lower_expr(t->as_dereference_value_unsafe()->getValue(), error);
    } else {
        addr = lower_address_of(t, error);
    }
    if (!addr.ok() || (addr.kind != MIRExprKind::Value && addr.kind != MIRExprKind::Address)) {
        error = "increment/decrement target is not an addressable lvalue";
        return MIRExprResult::error();
    }
    const ValueId ptr = static_cast<ValueId>(addr.id);
    const ValueId cur = builder_->load_indirect(ptr, vt);
    const ValueId nv = builder_->binary(cur, one, opc, vt);
    builder_->store_indirect(ptr, nv);
    return MIRExprResult::value(post ? cur : nv, vt);
}

bool MIRLowerer::lower_scope(Scope& scope, std::string& error) {
    return lower_scope_nodes_scoped(scope.nodes, scope.encoded_location().encoded, error);
}

bool MIRLowerer::lower_scope_nodes_scoped(std::vector<ASTNode*>& nodes, uint64_t encoded_loc,
                                          std::string& error) {
    if (!debug_info_ || !builder_) return lower_scope_nodes(nodes, error);
    // The builder is captured here because lowering a nested function (a lambda)
    // swaps `builder_` for its own and restores it before returning; the matching
    // pop must go to the builder that recorded the push.
    MIRBuilder* const b = builder_;
    MIRSourceLoc loc{};
    const bool has_loc =
        encoded_loc != 0 && source_loc_provider_ && source_loc_provider_(encoded_loc, loc);
    b->push_lexical_scope(loc, has_loc);
    const bool ok = lower_scope_nodes(nodes, error);
    b->pop_lexical_scope();
    return ok;
}

bool MIRLowerer::lower_scope_nodes(std::vector<ASTNode*>& nodes, std::string& error) {
    const size_t mark = destructibles_.size();
    for (ASTNode* n : nodes) {
        // statements after a terminator (break/continue/return) are unreachable
        if (builder_->current_block_terminated()) break;
        const size_t smark = stmt_temps_.size();
        if (!lower_stmt(n, error)) return false;
        if (!builder_->current_block_terminated()) flush_stmt_temps(smark);
        else stmt_temps_.resize(smark);
    }
    // destroy locals declared in this scope (unless they were moved out)
    if (!builder_->current_block_terminated()) {
        emit_drops_from(mark);
    }
    destructibles_.resize(mark);
    return true;
}

bool MIRLowerer::lower_if(IfStatement* stmt, BlockId merge, std::string& error) {
    const BlockId then_b = builder_->create_block();
    const BlockId else_b = builder_->create_block();

    MIRExprResult c = lower_expr(stmt->condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, then_b, else_b);

    builder_->set_block(then_b);
    if (!lower_scope(stmt->ifBody, error)) return false;
    if (!builder_->current_block_terminated()) builder_->br(merge);

    BlockId current = else_b;
    for (size_t i = 0; i < stmt->elseIfs.size(); ++i) {
        const BlockId body_b = builder_->create_block();
        const BlockId next_else = builder_->create_block();
        builder_->set_block(current);
        MIRExprResult ec = lower_expr(stmt->elseIfs[i].first, error);
        if (!ec.ok()) return false;
        builder_->cond_br(ec.id, body_b, next_else);
        builder_->set_block(body_b);
        if (!lower_scope(stmt->elseIfs[i].second, error)) return false;
        if (!builder_->current_block_terminated()) builder_->br(merge);
        current = next_else;
    }

    builder_->set_block(current);
    if (stmt->elseBody.has_value()) {
        if (!lower_scope(stmt->elseBody.value(), error)) return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(merge);
    return true;
}

bool MIRLowerer::lower_scope_value(Scope& scope, PlaceId dest, TypeId dest_t,
                                   std::string& error) {
    // Debug info: this scope body is a lexical block (see lower_scope_nodes_scoped).
    // The builder is captured because lowering a lambda inside the body swaps and
    // restores `builder_`; the pop must reach the builder that pushed.
    MIRBuilder* const ls_builder = debug_info_ ? builder_ : nullptr;
    if (ls_builder) {
        MIRSourceLoc ls_loc{};
        const uint64_t enc = scope.encoded_location().encoded;
        const bool has_loc =
            enc != 0 && source_loc_provider_ && source_loc_provider_(enc, ls_loc);
        ls_builder->push_lexical_scope(ls_loc, has_loc);
    }
    const size_t mark = destructibles_.size();
    const size_t smark = stmt_temps_.size();
    auto finish = [&]() -> bool {
        if (!builder_->current_block_terminated()) {
            emit_drops_from(mark);
            flush_stmt_temps(smark);
        } else {
            stmt_temps_.resize(smark);
        }
        destructibles_.resize(mark);
        if (ls_builder) ls_builder->pop_lexical_scope();
        return true;
    };

    const size_t n = scope.nodes.size();
    for (size_t i = 0; i < n; ++i) {
        if (builder_->current_block_terminated()) break;
        ASTNode* node = scope.nodes[i];
        if (i + 1 == n) {
            // the trailing node of a value scope may be a statement-form
            // if/switch/loop that must be lowered in value position
            Value* v = nullptr;
            switch (node->kind()) {
                case ASTNodeKind::IfStmt:
                    return lower_if_stmt_into(*node->as_if_stmt_unsafe(), dest, dest_t, error) &&
                           finish();
                case ASTNodeKind::SwitchStmt:
                    return lower_switch_into(*node->as_switch_stmt_unsafe(), dest, dest_t, error) &&
                           finish();
                case ASTNodeKind::LoopBlock:
                    return lower_loop_block_into(*node->as_loop_block_unsafe(), dest, dest_t, error) &&
                           finish();
                case ASTNodeKind::AccessChainNode:
                    v = &node->as_access_chain_node_unsafe()->chain;
                    break;
                default:
                    v = Value::get_first_value_from_value_node(node);
                    break;
            }
            if (v) {
                // a moved branch value (a destructured variant payload) consumes
                // the source it was moved from
                if (v->is_ref_moved()) clear_moved_ref(v);
                MIRExprResult r = lower_expr(v, error);
                if (!r.ok()) return false;
                if (r.kind == MIRExprKind::Place) {
                    // the trailing local is moved out into the result, so its
                    // destructor must not run at scope exit
                    mark_moved(static_cast<PlaceId>(r.id));
                    builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), dest_t));
                } else if (r.kind == MIRExprKind::Address) {
                    builder_->store(dest, builder_->load_indirect(static_cast<ValueId>(r.id), dest_t));
                } else if (r.kind == MIRExprKind::Value) {
                    builder_->store(dest, static_cast<ValueId>(r.id));
                } else {
                    error = "if-value branch does not produce a value";
                    return false;
                }
                break;
            }
        }
        if (!lower_stmt(node, error)) return false;
    }
    return finish();
}

bool MIRLowerer::lower_switch_dispatch(SwitchStatement& sw,
                                       std::vector<BlockId>& scope_blocks,
                                       BlockId default_b, std::string& error) {
    MIRExprResult scrut = lower_expr(sw.expression, error);
    if (!scrut.ok()) return false;

    // variant switch: compare the discriminant, not the whole value
    const std::string tag_name("__chx__vt_621827");
    const ConstantId tag_fc = module_.constants.add_string(
        MIR_INVALID_ID, tag_name.data(), static_cast<uint32_t>(tag_name.size()));
    const TypeId tag_t = types_.u32_type();
    bool variant_switch = false;
    if (sw.expression && sw.expression->getType()) {
        BaseType* ct = sw.expression->getType()->canonical();
        if (ct && ct->kind() == BaseTypeKind::Reference) {
            ct = ct->as_reference_type()->type->canonical();
        } else if (ct && ct->kind() == BaseTypeKind::Pointer) {
            ct = ct->as_pointer_type()->type->canonical();
        }
        ASTNode* ln = ct ? ct->get_direct_linked_canonical_node() : nullptr;
        if (ln && (ln->kind() == ASTNodeKind::VariantDecl ||
                   ln->kind() == ASTNodeKind::VariantMember)) {
            ValueId tag = MIR_NULL;
            if (scrut.kind == MIRExprKind::Place) {
                tag = builder_->field_load(static_cast<PlaceId>(scrut.id), tag_fc, tag_t);
            } else if (scrut.kind == MIRExprKind::Address ||
                       scrut.kind == MIRExprKind::Value) {
                tag = builder_->field_load_ptr(static_cast<ValueId>(scrut.id), tag_fc, tag_t);
            } else {
                error = "variant switch expression is not a variant lvalue";
                return false;
            }
            scrut = MIRExprResult::value(tag, tag_t);
            variant_switch = true;
        }
    }

    const TypeId bool_t = types_.bool_type();
    const ConstantId eqc = module_.constants.add_int(
        MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq));

    const size_t n = sw.cases.size();
    for (size_t i = 0; i < n; ++i) {
        auto& c = sw.cases[i];
        const BlockId target = (c.second >= 0) ? scope_blocks[c.second] : default_b;
        if (c.first == nullptr) {
            builder_->br(target);
            break;
        }
        const BlockId next_b = (i + 1 < n) ? builder_->create_block() : default_b;
        MIRExprResult cv;
        if (variant_switch && c.first->val_kind() == ValueKind::VariantCase) {
            auto* vc = c.first->as_variant_case_unsafe();
            const long idx =
                (vc->member && vc->member->parent())
                    ? vc->member->parent()->direct_child_index(vc->member->name)
                    : 0;
            const ValueId ci = builder_->const_int(
                tag_t, module_.constants.add_int(
                           tag_t, static_cast<uint64_t>(idx < 0 ? 0 : idx)));
            cv = MIRExprResult::value(ci, tag_t);
        } else {
            cv = lower_expr(c.first, error);
        }
        if (!cv.ok()) return false;
        const ValueId cond = builder_->compare(scrut.id, cv.id, eqc, bool_t);
        builder_->cond_br(cond, target, next_b);
        if (i + 1 < n) {
            builder_->set_block(next_b);
        } else {
            break;
        }
    }
    if (n == 0) builder_->br(default_b);
    return true;
}

MIRExprResult MIRLowerer::lower_switch_value(SwitchValue* value, std::string& error) {
    SwitchStatement& sw = value->stmt;
    const TypeId vt = types_.map(value->getType());
    const PlaceId dest = builder_->alloca(vt, MIRStorageClass::Temporary);
    if (!lower_switch_into(sw, dest, vt, error)) return MIRExprResult::error();
    if (abi::is_aggregate(module_, vt)) {
        return MIRExprResult::place(dest, vt);
    }
    return MIRExprResult::value(builder_->load(dest, vt), vt);
}

bool MIRLowerer::lower_switch_into(SwitchStatement& sw, PlaceId dest, TypeId vt,
                                   std::string& error) {
    const BlockId end_b = builder_->create_block();
    std::vector<BlockId> scope_blocks(sw.scopes.size());
    for (size_t i = 0; i < sw.scopes.size(); ++i) {
        scope_blocks[i] = builder_->create_block();
    }
    const BlockId default_b = (sw.defScopeInd >= 0) ? scope_blocks[sw.defScopeInd] : end_b;
    if (!lower_switch_dispatch(sw, scope_blocks, default_b, error)) return false;
    for (size_t i = 0; i < sw.scopes.size(); ++i) {
        builder_->set_block(scope_blocks[i]);
        if (!lower_scope_value(sw.scopes[i], dest, vt, error)) return false;
        if (!builder_->current_block_terminated()) builder_->br(end_b);
    }
    builder_->set_block(end_b);
    return true;
}

MIRExprResult MIRLowerer::lower_if_value(IfValue* value, std::string& error) {
    const TypeId vt = types_.map(value->getType());
    const PlaceId dest = builder_->alloca(vt, MIRStorageClass::Temporary);
    if (!lower_if_stmt_into(value->stmt, dest, vt, error)) return MIRExprResult::error();
    // for aggregates the caller wants the temp's address; for scalars the value
    if (abi::is_aggregate(module_, vt)) {
        return MIRExprResult::place(dest, vt);
    }
    return MIRExprResult::value(builder_->load(dest, vt), vt);
}

bool MIRLowerer::lower_if_stmt_into(IfStatement& stmt, PlaceId dest, TypeId vt,
                                    std::string& error) {
    const BlockId merge = builder_->create_block();
    const BlockId then_b = builder_->create_block();
    const BlockId else_b = builder_->create_block();
    MIRExprResult c = lower_expr(stmt.condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, then_b, else_b);

    builder_->set_block(then_b);
    if (!lower_scope_value(stmt.ifBody, dest, vt, error)) return false;
    if (!builder_->current_block_terminated()) builder_->br(merge);

    BlockId current = else_b;
    for (size_t i = 0; i < stmt.elseIfs.size(); ++i) {
        const BlockId body_b = builder_->create_block();
        const BlockId next_else = builder_->create_block();
        builder_->set_block(current);
        MIRExprResult ec = lower_expr(stmt.elseIfs[i].first, error);
        if (!ec.ok()) return false;
        builder_->cond_br(ec.id, body_b, next_else);
        builder_->set_block(body_b);
        if (!lower_scope_value(stmt.elseIfs[i].second, dest, vt, error)) return false;
        if (!builder_->current_block_terminated()) builder_->br(merge);
        current = next_else;
    }

    builder_->set_block(current);
    if (stmt.elseBody.has_value()) {
        if (!lower_scope_value(stmt.elseBody.value(), dest, vt, error)) return false;
    } else {
        error = "if-value has no else branch";
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(merge);

    builder_->set_block(merge);
    return true;
}

MIRExprResult MIRLowerer::lower_loop_value(LoopValue* value, std::string& error) {
    const TypeId vt = types_.map(value->getType());
    const PlaceId dest = builder_->alloca(vt, MIRStorageClass::Temporary);
    if (!lower_loop_block_into(value->stmt, dest, vt, error)) {
        return MIRExprResult::error();
    }
    if (abi::is_aggregate(module_, vt)) {
        return MIRExprResult::place(dest, vt);
    }
    return MIRExprResult::value(builder_->load(dest, vt), vt);
}

bool MIRLowerer::lower_loop_block_into(LoopBlock& loop, PlaceId dest, TypeId vt,
                                       std::string& error) {
    const BlockId body_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(body_b);
    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(body_b);
    loop_destructible_marks_.push_back(destructibles_.size());
    break_value_dests_.push_back(dest);
    break_value_types_.push_back(vt);
    if (!lower_scope(loop.body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(body_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();
    loop_destructible_marks_.pop_back();
    break_value_dests_.pop_back();
    break_value_types_.pop_back();

    builder_->set_block(exit_b);
    return true;
}

bool MIRLowerer::lower_loop_block(LoopBlock* loop, std::string& error) {
    return lower_loop_block_into(*loop, MIR_INVALID_ID, MIR_INVALID_ID, error);
}

bool MIRLowerer::lower_while(WhileLoop* loop, std::string& error) {
    const BlockId cond_b = builder_->create_block();
    const BlockId body_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(cond_b);
    builder_->set_block(cond_b);
    MIRExprResult c = lower_expr(loop->condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, body_b, exit_b);

    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(cond_b);
    loop_destructible_marks_.push_back(destructibles_.size());
    break_value_dests_.push_back(MIR_INVALID_ID);
    break_value_types_.push_back(MIR_INVALID_ID);
    if (!lower_scope(loop->body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(cond_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();
    loop_destructible_marks_.pop_back();
    break_value_dests_.pop_back();
    break_value_types_.pop_back();

    builder_->set_block(exit_b);
    return true;
}

bool MIRLowerer::lower_do_while(DoWhileLoop* loop, std::string& error) {
    const BlockId body_b = builder_->create_block();
    const BlockId cond_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(body_b);
    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(cond_b);
    loop_destructible_marks_.push_back(destructibles_.size());
    break_value_dests_.push_back(MIR_INVALID_ID);
    break_value_types_.push_back(MIR_INVALID_ID);
    if (!lower_scope(loop->body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(cond_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();
    loop_destructible_marks_.pop_back();
    break_value_dests_.pop_back();
    break_value_types_.pop_back();

    builder_->set_block(cond_b);
    MIRExprResult c = lower_expr(loop->condition, error);
    if (!c.ok()) return false;
    builder_->cond_br(c.id, body_b, exit_b);

    builder_->set_block(exit_b);
    return true;
}

bool MIRLowerer::lower_for(ForLoop* loop, std::string& error) {
    if (loop->initializer) {
        if (!lower_stmt(loop->initializer, error)) return false;
    }

    const BlockId cond_b = builder_->create_block();
    const BlockId body_b = builder_->create_block();
    const BlockId step_b = builder_->create_block();
    const BlockId exit_b = builder_->create_block();

    builder_->br(cond_b);
    builder_->set_block(cond_b);
    if (loop->conditionExpr) {
        MIRExprResult c = lower_expr(loop->conditionExpr, error);
        if (!c.ok()) return false;
        builder_->cond_br(c.id, body_b, exit_b);
    } else {
        builder_->br(body_b);
    }

    builder_->set_block(body_b);
    break_targets_.push_back(exit_b);
    continue_targets_.push_back(step_b);
    loop_destructible_marks_.push_back(destructibles_.size());
    break_value_dests_.push_back(MIR_INVALID_ID);
    break_value_types_.push_back(MIR_INVALID_ID);
    if (!lower_scope(loop->body, error)) {
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(step_b);
    break_targets_.pop_back();
    continue_targets_.pop_back();
    loop_destructible_marks_.pop_back();
    break_value_dests_.pop_back();
    break_value_types_.pop_back();

    builder_->set_block(step_b);
    if (loop->incrementerExpr) {
        if (!lower_stmt(loop->incrementerExpr, error)) return false;
    }
    if (!builder_->current_block_terminated()) builder_->br(cond_b);

    builder_->set_block(exit_b);
    return true;
}

FunctionDeclaration* MIRLowerer::resolve_iter_impl(BaseType* container_type, const char* which) {
    if (!container_type || !impls_index_ || !core_nodes_) return nullptr;
    ASTNode* node = container_type->get_linked_node(true, false);
    if (!node) node = container_type->get_direct_linked_canonical_node();
    if (!node) return nullptr;
    MembersContainer* container = node->get_members_container();
    if (!container) return nullptr;
    const std::string w(which);
    if (w == "data") return impls_index_->get_linear_data_impl(*core_nodes_, container);
    if (w == "size") return impls_index_->get_linear_size_impl(*core_nodes_, container);
    if (w == "begin") return impls_index_->get_iterable_begin_impl(*core_nodes_, container);
    if (w == "valid") return impls_index_->get_iterable_valid_impl(*core_nodes_, container);
    if (w == "current") return impls_index_->get_iterable_current_impl(*core_nodes_, container);
    if (w == "next") return impls_index_->get_iterable_next_impl(*core_nodes_, container);
    if (w == "rbegin")
        return impls_index_->get_reversible_iterable_rbegin_impl(*core_nodes_, container);
    if (w == "previous")
        return impls_index_->get_reversible_iterable_previous_impl(*core_nodes_, container);
    if (w == "count")
        return impls_index_->get_reversible_iterable_count_impl(*core_nodes_, container);
    if (w == "begin_chunks")
        return impls_index_->get_chunked_begin_chunks_impl(*core_nodes_, container);
    if (w == "valid_chunk")
        return impls_index_->get_chunked_valid_chunk_impl(*core_nodes_, container);
    if (w == "current_chunk")
        return impls_index_->get_chunked_current_chunk_impl(*core_nodes_, container);
    if (w == "next_chunk")
        return impls_index_->get_chunked_next_chunk_impl(*core_nodes_, container);
    if (w == "rbegin_chunks")
        return impls_index_->get_chunked_rbegin_chunks_impl(*core_nodes_, container);
    if (w == "previous_chunk")
        return impls_index_->get_chunked_previous_chunk_impl(*core_nodes_, container);
    if (w == "total_size")
        return impls_index_->get_chunked_total_size_impl(*core_nodes_, container);
    return nullptr;
}

MIRExprResult MIRLowerer::call_method_values(FunctionDeclaration* fd, MIRExprResult recv,
                                             BaseType* recv_type,
                                             const std::vector<Value*>& args,
                                             std::string& error) {
    if (!fd) {
        error = "iterable method could not be resolved";
        return MIRExprResult::error();
    }
    VariableIdentifier id(chem::string_view(), nullptr, ZERO_LOC);
    id.linked = fd;
    FunctionCall call(&id, fd->returnType, ZERO_LOC);
    call.values = args;
    return lower_method_call(recv, recv_type, &call, error);
}

bool MIRLowerer::lower_for_in(ForInLoop* loop, std::string& error) {
    MIRExprResult it = lower_expr(loop->expr, error);
    if (!it.ok()) return false;
    BaseType* expr_ast = loop->expr->getType();
    BaseType* expr_c = expr_ast ? expr_ast->canonical() : nullptr;
    if (!expr_c) {
        error = "for-in iterable has no type";
        return false;
    }
    const bool is_array = expr_c->kind() == BaseTypeKind::Array;
    const bool reversed = loop->is_reversed();

    // a stable place for the iterable (the protocol methods take `&self`)
    PlaceId recv = MIR_INVALID_ID;
    TypeId recv_t = MIR_INVALID_ID;
    if (it.kind == MIRExprKind::Place) {
        recv = static_cast<PlaceId>(it.id);
        recv_t = builder_->function().places[recv].type;
    } else if (it.kind == MIRExprKind::Value || it.kind == MIRExprKind::Address) {
        recv_t = it.type;
        recv = builder_->alloca(recv_t, MIRStorageClass::Temporary);
        builder_->store(recv, static_cast<ValueId>(it.id));
    } else {
        error = "for-in iterable does not produce a value";
        return false;
    }
    const MIRExprResult recv_place = MIRExprResult::place(recv, recv_t);

    const TypeId elem_t = types_.map(loop->getIterationElementActualType());
    const TypeId elem_ptr_t = types_.pointer_type(elem_t, true);
    const TypeId u64t = types_.u64_type();
    const TypeId bool_t = types_.bool_type();
    const TypeId u32t = types_.u32_type();

    // the loop variable holds a pointer to the current element
    const PlaceId var_place = builder_->alloca(elem_ptr_t, MIRStorageClass::Temporary);
    bind(loop, var_place);
    bind_name(loop->id, var_place);

    PlaceId idx_place = MIR_INVALID_ID;
    TypeId idx_t = MIR_INVALID_ID;
    if (loop->index_init) {
        idx_t = types_.map(loop->index_init->known_type());
        idx_place = builder_->alloca(idx_t, MIRStorageClass::Temporary);
        bind(loop->index_init, idx_place);
        bind_name(loop->index_init->id_view(), idx_place);
    }

    const ConstantId add_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Add));
    const ConstantId sub_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Sub));
    const ConstantId ne_op =
        module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Ne));
    const ValueId one32 = builder_->const_int(u32t, module_.constants.add_int(u32t, 1));
    static uint64_t s_forin_counter = 0;
    const std::string uniq = std::to_string(s_forin_counter++);

    auto emit_idx_step = [&](bool increment) {
        if (idx_place == MIR_INVALID_ID) return;
        const ValueId cur = builder_->load(idx_place, idx_t);
        const ValueId one = builder_->const_int(idx_t, module_.constants.add_int(idx_t, 1));
        builder_->store(idx_place,
                        builder_->binary(cur, one, increment ? add_op : sub_op, idx_t));
    };
    auto emit_var_step = [&](bool increment) {
        const ValueId cur = builder_->load(var_place, elem_ptr_t);
        builder_->store(var_place,
                        builder_->binary(cur, one32, increment ? add_op : sub_op, elem_ptr_t));
    };
    auto store_ptr = [&](PlaceId dest, MIRExprResult r, TypeId dest_t) -> bool {
        if (r.kind == MIRExprKind::Place) {
            builder_->store(dest, builder_->load(static_cast<PlaceId>(r.id), r.type));
        } else if (r.kind == MIRExprKind::Value || r.kind == MIRExprKind::Address) {
            builder_->store(dest, static_cast<ValueId>(r.id));
        } else {
            error = "for-in expression does not produce a value";
            return false;
        }
        return true;
    };

    // run one iteration structure: `cond_b` already current
    auto run_loop = [&](BlockId cond_b, auto&& emit_cond, auto&& emit_init,
                        auto&& emit_step) -> bool {
        const BlockId body_b = builder_->create_block();
        const BlockId step_b = builder_->create_block();
        const BlockId exit_b = builder_->create_block();
        builder_->br(cond_b);
        builder_->set_block(cond_b);
        if (!emit_cond(body_b, exit_b)) return false;
        builder_->set_block(body_b);
        if (!emit_init()) return false;
        break_targets_.push_back(exit_b);
        continue_targets_.push_back(step_b);
        loop_destructible_marks_.push_back(destructibles_.size());
        break_value_dests_.push_back(MIR_INVALID_ID);
        break_value_types_.push_back(MIR_INVALID_ID);
        if (!lower_scope(loop->body, error)) {
            break_targets_.pop_back();
            continue_targets_.pop_back();
            loop_destructible_marks_.pop_back();
            break_value_dests_.pop_back();
            break_value_types_.pop_back();
            return false;
        }
        if (!builder_->current_block_terminated()) builder_->br(step_b);
        break_targets_.pop_back();
        continue_targets_.pop_back();
        loop_destructible_marks_.pop_back();
        break_value_dests_.pop_back();
        break_value_types_.pop_back();
        builder_->set_block(step_b);
        if (!emit_step()) return false;
        if (!builder_->current_block_terminated()) builder_->br(cond_b);
        builder_->set_block(exit_b);
        return true;
    };

    if (is_array) {
        const uint64_t n = expr_c->as_array_type_unsafe()->get_array_size();
        const MIRTypeRecord& rr = module_.types.get(recv_t);
        ValueId data_v;
        if (rr.kind == MIRTypeKind::Pointer || rr.kind == MIRTypeKind::Reference) {
            // an array parameter decays to a pointer to its first element
            data_v = builder_->load(recv, recv_t);
            if (data_v != MIR_NULL && recv_t != elem_ptr_t) {
                data_v = builder_->cast(data_v, elem_ptr_t);
            }
        } else {
            data_v = builder_->address_of(recv, elem_ptr_t);
        }
        const ValueId size_v = builder_->const_int(u64t, module_.constants.add_int(u64t, n));
        const ValueId end_v = builder_->binary(data_v, size_v, add_op, elem_ptr_t);
        const PlaceId data_place = builder_->alloca(elem_ptr_t, MIRStorageClass::Temporary);
        builder_->store(data_place, data_v);
        if (idx_place != MIR_INVALID_ID) {
            if (loop->is_reversed_counter()) {
                builder_->store(idx_place, size_v);
            } else {
                builder_->store(idx_place,
                                builder_->const_int(idx_t, module_.constants.add_int(idx_t, 0)));
            }
        }
        if (reversed) {
            builder_->store(var_place, end_v);
        } else {
            builder_->store(var_place, data_v);
        }
        const BlockId cond_b = builder_->create_block();
        return run_loop(
            cond_b,
            [&](BlockId body_b, BlockId exit_b) -> bool {
                const ValueId v = builder_->load(var_place, elem_ptr_t);
                const ValueId e = reversed ? builder_->load(data_place, elem_ptr_t) : end_v;
                builder_->cond_br(builder_->compare(v, e, ne_op, bool_t), body_b, exit_b);
                return true;
            },
            [&]() -> bool {
                if (reversed) {
                    emit_idx_step(false);
                    emit_var_step(false);
                }
                return true;
            },
            [&]() -> bool {
                if (!reversed) {
                    emit_idx_step(true);
                    emit_var_step(true);
                }
                return true;
            });
    }

    // Linear iteration (data/size)
    FunctionDeclaration* data_fd = resolve_iter_impl(expr_c, "data");
    FunctionDeclaration* size_fd = resolve_iter_impl(expr_c, "size");
    if (data_fd && size_fd) {
        MIRExprResult dr = call_method_values(data_fd, recv_place, expr_c, {}, error);
        if (!dr.ok()) return false;
        MIRExprResult sr = call_method_values(size_fd, recv_place, expr_c, {}, error);
        if (!sr.ok()) return false;
        ValueId data_v = (dr.kind == MIRExprKind::Place)
                             ? builder_->load(static_cast<PlaceId>(dr.id), dr.type)
                             : static_cast<ValueId>(dr.id);
        ValueId size_v = (sr.kind == MIRExprKind::Place)
                             ? builder_->load(static_cast<PlaceId>(sr.id), sr.type)
                             : static_cast<ValueId>(sr.id);
        if (data_v != MIR_NULL && dr.type != elem_ptr_t) data_v = builder_->cast(data_v, elem_ptr_t);
        const ValueId end_v = builder_->binary(data_v, size_v, add_op, elem_ptr_t);
        const PlaceId data_place = builder_->alloca(elem_ptr_t, MIRStorageClass::Temporary);
        builder_->store(data_place, data_v);
        if (idx_place != MIR_INVALID_ID) {
            if (loop->is_reversed_counter()) {
                builder_->store(idx_place, size_v);
            } else {
                builder_->store(idx_place,
                                builder_->const_int(idx_t, module_.constants.add_int(idx_t, 0)));
            }
        }
        if (reversed) {
            builder_->store(var_place, end_v);
        } else {
            builder_->store(var_place, data_v);
        }
        const BlockId cond_b = builder_->create_block();
        return run_loop(
            cond_b,
            [&](BlockId body_b, BlockId exit_b) -> bool {
                const ValueId v = builder_->load(var_place, elem_ptr_t);
                const ValueId e = reversed ? builder_->load(data_place, elem_ptr_t) : end_v;
                builder_->cond_br(v == MIR_NULL ? v : builder_->compare(v, e, ne_op, bool_t),
                                  body_b, exit_b);
                return true;
            },
            [&]() -> bool {
                if (reversed) {
                    emit_idx_step(false);
                    emit_var_step(false);
                }
                return true;
            },
            [&]() -> bool {
                if (!reversed) {
                    emit_idx_step(true);
                    emit_var_step(true);
                }
                return true;
            });
    }

    // Chunked iteration (begin_chunks / valid_chunk / current_chunk / next_chunk)
    {
        FunctionDeclaration* begin_fd =
            resolve_iter_impl(expr_c, reversed ? "rbegin_chunks" : "begin_chunks");
        FunctionDeclaration* valid_fd = resolve_iter_impl(expr_c, "valid_chunk");
        FunctionDeclaration* current_fd = resolve_iter_impl(expr_c, "current_chunk");
        FunctionDeclaration* next_fd =
            resolve_iter_impl(expr_c, reversed ? "previous_chunk" : "next_chunk");
        if (begin_fd && valid_fd && current_fd && next_fd) {
            const TypeId cursor_t = types_.map(begin_fd->returnType);
            const PlaceId cursor_place = builder_->alloca(cursor_t, MIRStorageClass::Temporary);
            std::string cursor_name = "__chx_forin_chunk_cursor_" + uniq;
            bind_name(chem::string_view(cursor_name.data(), cursor_name.size()), cursor_place);
            MIRExprResult br = call_method_values(begin_fd, recv_place, expr_c, {}, error);
            if (!br.ok()) return false;
            if (!store_ptr(cursor_place, br, cursor_t)) return false;

            if (idx_place != MIR_INVALID_ID) {
                if (loop->is_reversed_counter()) {
                    FunctionDeclaration* total_fd = resolve_iter_impl(expr_c, "total_size");
                    MIRExprResult tr =
                        call_method_values(total_fd, recv_place, expr_c, {}, error);
                    if (!tr.ok()) return false;
                    if (!store_ptr(idx_place, tr, idx_t)) return false;
                } else {
                    builder_->store(
                        idx_place,
                        builder_->const_int(idx_t, module_.constants.add_int(idx_t, 0)));
                }
            }

            const BlockId ocond = builder_->create_block();
            const BlockId obody = builder_->create_block();
            const BlockId osteps = builder_->create_block();
            const BlockId oexit = builder_->create_block();
            builder_->br(ocond);
            builder_->set_block(ocond);
            {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult vr =
                    call_method_values(valid_fd, recv_place, expr_c, vargs, error);
                if (!vr.ok()) return false;
                builder_->cond_br(vr.id, obody, oexit);
            }
            builder_->set_block(obody);
            {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult cr =
                    call_method_values(current_fd, recv_place, expr_c, vargs, error);
                if (!cr.ok()) return false;
                const TypeId chunk_t = types_.map(current_fd->returnType);
                PlaceId chunk_place = MIR_INVALID_ID;
                if (cr.kind == MIRExprKind::Place) {
                    chunk_place = static_cast<PlaceId>(cr.id);
                } else {
                    chunk_place = builder_->alloca(chunk_t, MIRStorageClass::Temporary);
                    builder_->store(chunk_place, static_cast<ValueId>(cr.id));
                }
                const ConstantId fc_ptr = module_.constants.add_string(MIR_INVALID_ID, "ptr", 3);
                const ConstantId fc_len = module_.constants.add_string(MIR_INVALID_ID, "len", 3);
                const ValueId cptr = builder_->field_load(chunk_place, fc_ptr, elem_ptr_t);
                const ValueId clen = builder_->field_load(chunk_place, fc_len, u64t);
                const ValueId inner_end_v = builder_->binary(cptr, clen, add_op, elem_ptr_t);
                const PlaceId inner_end_place =
                    builder_->alloca(elem_ptr_t, MIRStorageClass::Temporary);
                if (reversed) {
                    builder_->store(inner_end_place, cptr);
                    builder_->store(var_place, inner_end_v);
                } else {
                    builder_->store(var_place, cptr);
                    builder_->store(inner_end_place, inner_end_v);
                }

                const BlockId icond = builder_->create_block();
                const BlockId ibody = builder_->create_block();
                const BlockId istep = builder_->create_block();
                builder_->br(icond);
                builder_->set_block(icond);
                {
                    const ValueId v = builder_->load(var_place, elem_ptr_t);
                    const ValueId e = builder_->load(inner_end_place, elem_ptr_t);
                    // normal inner completion advances the chunk; `break` exits
                    builder_->cond_br(builder_->compare(v, e, ne_op, bool_t), ibody, osteps);
                }
                builder_->set_block(ibody);
                if (reversed) {
                    emit_idx_step(false);
                    emit_var_step(false);
                }
                break_targets_.push_back(oexit);
                continue_targets_.push_back(istep);
                loop_destructible_marks_.push_back(destructibles_.size());
                break_value_dests_.push_back(MIR_INVALID_ID);
                break_value_types_.push_back(MIR_INVALID_ID);
                if (!lower_scope(loop->body, error)) {
                    break_targets_.pop_back();
                    continue_targets_.pop_back();
                    loop_destructible_marks_.pop_back();
                    break_value_dests_.pop_back();
                    break_value_types_.pop_back();
                    return false;
                }
                if (!builder_->current_block_terminated()) builder_->br(istep);
                break_targets_.pop_back();
                continue_targets_.pop_back();
                loop_destructible_marks_.pop_back();
                break_value_dests_.pop_back();
                break_value_types_.pop_back();
                builder_->set_block(istep);
                if (!reversed) {
                    emit_idx_step(true);
                    emit_var_step(true);
                }
                if (!builder_->current_block_terminated()) builder_->br(icond);
            }
            builder_->set_block(osteps);
            {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult nr =
                    call_method_values(next_fd, recv_place, expr_c, vargs, error);
                if (!nr.ok()) return false;
                if (!store_ptr(cursor_place, nr, cursor_t)) return false;
            }
            if (!builder_->current_block_terminated()) builder_->br(ocond);
            builder_->set_block(oexit);
            return true;
        }
    }

    // Iterable protocol (begin/valid/current/next)
    {
        FunctionDeclaration* begin_fd = resolve_iter_impl(expr_c, reversed ? "rbegin" : "begin");
        FunctionDeclaration* valid_fd = resolve_iter_impl(expr_c, "valid");
        FunctionDeclaration* current_fd = resolve_iter_impl(expr_c, "current");
        FunctionDeclaration* next_fd = resolve_iter_impl(expr_c, reversed ? "previous" : "next");
        if (!begin_fd || !valid_fd || !current_fd || !next_fd) {
            error = "container does not implement a supported iteration protocol";
            return false;
        }
        const TypeId cursor_t = types_.map(begin_fd->returnType);
        const PlaceId cursor_place = builder_->alloca(cursor_t, MIRStorageClass::Temporary);
        std::string cursor_name = "__chx_forin_cursor_" + uniq;
        bind_name(chem::string_view(cursor_name.data(), cursor_name.size()), cursor_place);
        MIRExprResult br = call_method_values(begin_fd, recv_place, expr_c, {}, error);
        if (!br.ok()) return false;
        if (!store_ptr(cursor_place, br, cursor_t)) return false;

        if (idx_place != MIR_INVALID_ID) {
            if (loop->is_reversed_counter()) {
                FunctionDeclaration* count_fd = resolve_iter_impl(expr_c, "count");
                MIRExprResult cr = call_method_values(count_fd, recv_place, expr_c, {}, error);
                if (!cr.ok()) return false;
                if (!store_ptr(idx_place, cr, idx_t)) return false;
            } else {
                builder_->store(idx_place,
                                builder_->const_int(idx_t, module_.constants.add_int(idx_t, 0)));
            }
        }
        const BlockId cond_b = builder_->create_block();
        return run_loop(
            cond_b,
            [&](BlockId body_b, BlockId exit_b) -> bool {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult vr = call_method_values(valid_fd, recv_place, expr_c, vargs, error);
                if (!vr.ok()) return false;
                builder_->cond_br(vr.id, body_b, exit_b);
                return true;
            },
            [&]() -> bool {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult cr = call_method_values(current_fd, recv_place, expr_c, vargs, error);
                if (!cr.ok()) return false;
                ValueId cv = (cr.kind == MIRExprKind::Place)
                                 ? builder_->load(static_cast<PlaceId>(cr.id), cr.type)
                                 : static_cast<ValueId>(cr.id);
                if (cv != MIR_NULL && cr.type != elem_ptr_t) cv = builder_->cast(cv, elem_ptr_t);
                builder_->store(var_place, cv);
                return true;
            },
            [&]() -> bool {
                VariableIdentifier cid(chem::string_view(cursor_name.data(), cursor_name.size()),
                                       begin_fd->returnType, ZERO_LOC);
                std::vector<Value*> vargs{&cid};
                MIRExprResult nr = call_method_values(next_fd, recv_place, expr_c, vargs, error);
                if (!nr.ok()) return false;
                if (!store_ptr(cursor_place, nr, cursor_t)) return false;
                emit_idx_step(!reversed);
                return true;
            });
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
    if (set_current_func_) set_current_func_(decl);
    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    stmt_temps_.clear();

    func.symbol = intern_function(decl);
    func.function_type = module_.symbols.symbols[func.symbol].type;

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
    if (decl->is_async()) {
        return lower_async_function(decl, arena, func, error);
    }
    arena_ = &arena;
    current_func_ = decl;
    if (set_current_func_) set_current_func_(decl);
    var_places_.clear();
    name_places_.clear();
    aggregate_param_ptrs_.clear();
    destructibles_.clear();
    stmt_temps_.clear();
    // func_symbols_ persists across functions in a module so callees keep stable ids

    func.symbol = intern_function(decl);
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
        std::vector<TypeId> ptypes;
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
        module_.symbols.symbols[func.symbol].type = ftype;
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
