// Copyright (c) Chemical Language Foundation 2025.

#include "MIRLowerer.h"

#include <cstdlib>
#include <iostream>
#include <optional>

#include "ast/base/BaseType.h"
#include "ast/types/GenericType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"
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
#include "MIREmitter.h"

namespace mir {

namespace {

bool is_void_type(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    return module.types.get(type).kind == MIRTypeKind::Void;
}

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
                fprintf(stderr, "[SIZEOF-RESOLVE] var %.*s\n",
                        (int)n->as_var_init_unsafe()->name_view().size(),
                        n->as_var_init_unsafe()->name_view().data());
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

bool needs_aggregate_path(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    const MIRTypeRecord& rec = module.types.get(type);
    if (rec.flags & TF_TYPEDEF) return false; // dynamic fat pointer: passed by value
    const MIRTypeKind k = rec.kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union || k == MIRTypeKind::Variant ||
           k == MIRTypeKind::Array;
}

/**
 * Whether a value of `type` is returned through a hidden sret pointer. This is
 * true for every struct-like type (including typedef structs such as the
 * dynamic fat pointer), matching the legacy prototype ABI. It differs from
 * `needs_aggregate_path`, which excludes typedef structs because those are
 * *passed by value*.
 */
bool needs_sret_return(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    const MIRTypeKind k = module.types.get(type).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union || k == MIRTypeKind::Variant ||
           k == MIRTypeKind::Array;
}

/** True for struct/union/variant types (operator overloads live on these). */
bool is_struct_like(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    const MIRTypeKind k = module.types.get(type).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union || k == MIRTypeKind::Variant;
}

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

bool map_binary(Operation op, MIROpcode& out, MIRBinaryOp& bop) {
    switch (op) {
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
    TypeId ftype = types_.function_signature(ret, ptypes);
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = decl->is_extern() ? MIRLinkage::External : MIRLinkage::Internal;
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
                if (ftr.kind == MIRTypeKind::Pointer || ftr.kind == MIRTypeKind::Reference) {
                    cur_ptr = builder_->load_indirect(faddr, ft);
                } else {
                    cur_ptr = faddr;
                }
                have_ptr = true;
            }
            const TypeId ptrt = types_.pointer_type(types_.map(t->getType()), true);
            return MIRExprResult::value(cur_ptr, ptrt);
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
            if (is_struct_like(module_, bt)) {
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
        const PlaceId p = place_for_linked(id->linked);
        if (p != MIR_INVALID_ID) {
            const TypeId pt = builder_->function().places[p].type;
            const MIRTypeRecord& pr = module_.types.get(pt);
            const TypeId ast_t = types_.map(id->getType());
            if (pr.kind == MIRTypeKind::Reference ||
                (needs_aggregate_path(module_, ast_t) && pr.kind == MIRTypeKind::Pointer)) {
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
                (needs_aggregate_path(module_, ast_t) && pr.kind == MIRTypeKind::Pointer)) {
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
    if (v->val_kind() != ValueKind::FunctionCall) return;
    BaseType* bt = v->getType();
    if (bt && bt->get_destructor()) {
        register_destructible(static_cast<PlaceId>(r.id), bt);
    }
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
    for (auto& d : destructibles_) {
        const ValueId fv = builder_->load(d.flag, types_.bool_type());
        builder_->drop(d.place, d.dtor, fv);
    }
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
                if (!lower_call_args_for(ctor, true, acall->values, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && !append_default_args(ctor, acall->values.size(), true, cargs, error)) {
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
        } else if (needs_aggregate_path(module_, pt)) {
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
    // a struct-returning call used as a member/index base (or directly as the
    // receiver of a chained method call) owns a temporary that must be
    // destroyed after the enclosing scope
    maybe_register_call_temp(values[0], first);
    if (count == 1) return first;
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
                if (needs_aggregate_path(module_, ft)) {
                    return MIRExprResult::address(builder_->field_addr(cur_place, fc, ft), ft);
                }
                return MIRExprResult::value(builder_->field_load(cur_place, fc, ft), ft);
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
                if (needs_aggregate_path(module_, ft)) {
                    return MIRExprResult::address(builder_->field_addr_ptr(cur_ptr, fc, ft), ft);
                }
                return MIRExprResult::value(builder_->field_load_ptr(cur_ptr, fc, ft), ft);
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
        if (as_address || needs_aggregate_path(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr(bp, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load(bp, fc, ft), ft);
    }
    if (base.kind == MIRExprKind::Address || base.kind == MIRExprKind::Value) {
        const ValueId bv = static_cast<ValueId>(base.id);
        if (as_address || needs_aggregate_path(module_, ft)) {
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
        if (as_address || needs_aggregate_path(module_, ft)) {
            return MIRExprResult::address(builder_->field_addr(bp, fc, ft), ft);
        }
        return MIRExprResult::value(builder_->field_load(bp, fc, ft), ft);
    }
    if (base.kind == MIRExprKind::Address || base.kind == MIRExprKind::Value) {
        const ValueId bv = static_cast<ValueId>(base.id);
        if (as_address || needs_aggregate_path(module_, ft)) {
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
        if (needs_aggregate_path(module_, pt)) return MIRExprResult::place(res, pt);
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
                if (needs_aggregate_path(module_, t)) return r;
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
        if (needs_aggregate_path(module_, dtype)) {
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
            if (needs_aggregate_path(module_, rt)) {
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
    if (needs_sret_return(module_, ret_type)) {
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
    rec.linkage = vi->is_extern() ? MIRLinkage::External : MIRLinkage::Internal;
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

    TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    std::vector<TypeId> ptypes;
    ptypes.reserve(decl->params.size());
    for (FunctionParam* p : decl->params) {
        ptypes.push_back(p && p->type ? types_.map(p->type) : types_.opaque_type());
    }
    TypeId ftype = types_.function_signature(ret, ptypes);

    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = decl->is_extern() ? MIRLinkage::External : MIRLinkage::Internal;
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
            if (id->linked && id->linked->kind() == ASTNodeKind::FunctionDecl) {
                // a function used as a value (function pointer)
                FunctionDeclaration* fd = id->linked->as_function_unsafe();
                const SymbolId sym = intern_function(fd);
                return MIRExprResult::value(builder_->function_addr(sym, type), type);
            }
            PlaceId place = place_for_linked(id->linked);
            if (place == MIR_INVALID_ID) place = place_for_name(id->value);
            if (place != MIR_INVALID_ID) {
                if (needs_aggregate_path(module_, type)) {
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
                if (needs_aggregate_path(module_, type)) {
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
                    if (needs_aggregate_path(module_, type)) {
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
            if (is_struct_like(module_, in.type)) {
                return lower_unary_operator_overload(n->getValue(), in, "neg", n->getType(), error);
            }
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::Neg));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::NotValue: {
            auto* n = value->as_not_value_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            if (is_struct_like(module_, in.type)) {
                return lower_unary_operator_overload(n->getValue(), in, "not", n->getType(), error);
            }
            ConstantId op = module_.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRUnaryOp::Not));
            return MIRExprResult::value(builder_->unary(in.id, op, type), type);
        }
        case ValueKind::BitwiseNot: {
            auto* n = value->as_bitwise_not_unsafe();
            MIRExprResult in = lower_expr(n->getValue(), error);
            if (!in.ok()) return in;
            if (is_struct_like(module_, in.type)) {
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
            if (is_struct_like(module_, lhs.type) || is_struct_like(module_, rhs.type)) {
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
            if (BaseType* ctype = value->getType()) {
                ASTNode* cn = ctype->get_direct_linked_canonical_node();
                if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                    sd = cn->as_struct_def_unsafe();
                }
            }
            if ((!sd || st == MIR_INVALID_ID || st >= module_.types.size() ||
                 module_.types.get(st).kind == MIRTypeKind::Opaque) &&
                expected_struct_type_) {
                st = types_.map(expected_struct_type_);
                ASTNode* cn = expected_struct_type_->get_direct_linked_canonical_node();
                if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                    sd = cn->as_struct_def_unsafe();
                }
            }
            if (!sd || st == MIR_INVALID_ID || st >= module_.types.size() ||
                module_.types.get(st).kind == MIRTypeKind::Opaque) {
                // e.g. a generic struct literal whose instantiation MIR has not
                // resolved: fall back rather than emit `void*` field stores
                error = "struct literal with an unresolved (generic?) type is not yet supported";
                return MIRExprResult::error();
            }
            const PlaceId temp = builder_->alloca(st, MIRStorageClass::Temporary);
            if (temp == MIR_INVALID_ID) {
                error = "failed to allocate struct temporary";
                return MIRExprResult::error();
            }
            // zero the temp so members that are not explicitly set are
            // well-defined (matches the legacy backend's default initialization)
            builder_->zero_init(temp);
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
                if (r.kind != MIRExprKind::Value) {
                    error = "unsupported struct field initializer";
                    return MIRExprResult::error();
                }
                // if the field is an aggregate value but the initializer is a
                // pointer/reference, dereference to get the value
                if (!member_type && sd) {
                    auto mtw2 = sd->variable_type_w_index(init.name);
                    if (mtw2.second != -1) member_type = mtw2.first;
                }
                if (member_type) {
                    const MIRTypeRecord& rt = module_.types.get(r.type);
                    if ((rt.kind == MIRTypeKind::Pointer ||
                         rt.kind == MIRTypeKind::Reference) &&
                        needs_aggregate_path(module_, types_.map(member_type))) {
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
            // values (the struct temp was zeroed above, so scalars are already 0)
            if (sd) {
                for (BaseDefMember* member : sd->variables()) {
                    if (!member) continue;
                    const chem::string_view mname = member->name;
                    if (sv->values.find(mname) != sv->values.end()) continue;
                    const ConstantId fc = module_.constants.add_string(
                        MIR_INVALID_ID, mname.data(), static_cast<uint32_t>(mname.size()));
                    Value* dv = member->default_value();
                    if (!dv) {
                        // a struct-typed member with a default constructor is
                        // constructed (matches the legacy default initialization)
                        BaseType* mt = member->known_type();
                        ASTNode* cn = mt ? mt->get_direct_linked_canonical_node() : nullptr;
                        if (cn && cn->kind() == ASTNodeKind::StructDecl) {
                            FunctionDeclaration* cons =
                                cn->as_struct_def_unsafe()->default_constructor_func();
                            if (cons && !cons->is_generated_fn()) {
                                const TypeId mtid = types_.map(mt);
                                const PlaceId tmp =
                                    builder_->alloca(mtid, MIRStorageClass::Temporary);
                                const SymbolId sym = intern_function(cons);
                                builder_->zero_init(tmp);
                                builder_->init(tmp, sym, nullptr, 0);
                                builder_->field_store_place(temp, fc, tmp);
                            }
                        }
                        continue;
                    }
                    MIRExprResult r = lower_expr(dv, error);
                    if (!r.ok()) return r;
                    if (r.kind == MIRExprKind::Place) {
                        builder_->field_store_place(temp, fc, static_cast<PlaceId>(r.id));
                    } else if (r.kind == MIRExprKind::Value) {
                        builder_->field_store(temp, fc, r.id);
                    }
                }
            }
            return MIRExprResult::place(temp, st);
        }
        case ValueKind::FunctionCall: {
            auto* call = value->as_func_call_unsafe();
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
                    builder_->zero_init(temp);
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
                    return MIRExprResult::place(temp, vt);
                }
            }

            // aggregate construction: `Type(args)` / `ns.Type(args)` /
            // `ns.Generic<...>(args)`
            ASTNode* ctor_node = nullptr;
            if (call->parent_val) {
                ASTNode* plk = call->parent_val->linked_node();
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
                if (!lower_call_args_for(ctor, true, call->values, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor && !append_default_args(ctor, call->values.size(), true, cargs, error)) {
                    return MIRExprResult::error();
                }
                if (ctor) {
                    const SymbolId sym = intern_function(ctor);
                    builder_->zero_init(place);
                    builder_->init(place, sym, cargs.data(), static_cast<uint32_t>(cargs.size()));
                } else if (!cargs.empty()) {
                    error = "no matching constructor for aggregate construction";
                    return MIRExprResult::error();
                }
                return MIRExprResult::place(place, st);
            }

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
                    if (!mfd && !recv_is_place && needs_aggregate_path(module_, ct)) {
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
                        if (!lower_call_args_for(ctor, true, call->values, cargs, error)) {
                            return MIRExprResult::error();
                        }
                        if (ctor && !append_default_args(ctor, call->values.size(), true, cargs,
                                                         error)) {
                            return MIRExprResult::error();
                        }
                        if (ctor && ctor->is_comptime() && comptime_eval_) {
                            Value* ev = comptime_eval_(call, ctor);
                            if (ev) return lower_expr(ev, error);
                        }
                        if (ctor) {
                            const SymbolId sym = intern_function(ctor);
                            builder_->zero_init(place);
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
                    // otherwise: namespace/type-qualified free function or a
                    // static method -- fall through to the direct-call path,
                    // which passes no receiver.
                }
            }
            {
                ASTNode* linked_fn = call->parent_val ? call->parent_val->linked_node() : nullptr;
                fd = linked_fn ? linked_fn->as_function() : nullptr;
                if (!fd) {
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
                    if (needs_sret_return(module_, rt)) {
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
            if (needs_sret_return(module_, type)) {
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
            // zero the array so elements not covered by the literal are 0
            builder_->zero_init(place);
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
                    if (needs_aggregate_path(module_, pt)) {
                        builder_->element_store_place(place, idx, pid);
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
                    if (needs_aggregate_path(module_, et)) {
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
            if (base.kind == MIRExprKind::Place) {
                const TypeId bt = builder_->function().places[base.id].type;
                if (is_struct_like(module_, bt)) {
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
                        if (needs_aggregate_path(module_, type)) {
                            return MIRExprResult::address(static_cast<ValueId>(r.id), type);
                        }
                        return MIRExprResult::value(
                            builder_->load_indirect(static_cast<ValueId>(r.id), type), type);
                    }
                    return r;
                }
            }
            if (needs_aggregate_path(module_, type)) {
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
            if (needs_aggregate_path(module_, type)) {
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
                    for (size_t i = mark; i < destructibles_.size(); ++i) {
                        const ValueId fv =
                            builder_->load(destructibles_[i].flag, types_.bool_type());
                        builder_->drop(destructibles_[i].place, destructibles_[i].dtor, fv);
                    }
                }
                destructibles_.resize(mark);
            } else if (!lower_scope_value(*scope, dest, vt, error)) {
                return MIRExprResult::error();
            }
            if (needs_aggregate_path(module_, vt)) {
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
            if (needs_aggregate_path(module_, zt)) return MIRExprResult::place(tmp, zt);
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
            builder_->zero_init(temp);
            MIRExprResult obj = lower_address_of(dv->value, error);
            if (!obj.ok()) return obj;
            ValueId obj_ptr = MIR_NULL;
            if (obj.kind == MIRExprKind::Place) {
                const TypeId pt = builder_->function().places[obj.id].type;
                obj_ptr = builder_->address_of(obj.id, types_.pointer_type(pt, true));
            } else if (obj.kind == MIRExprKind::Value || obj.kind == MIRExprKind::Address) {
                obj_ptr = static_cast<ValueId>(obj.id);
            } else {
                error = "dynamic value operand is not addressable";
                return MIRExprResult::error();
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

bool MIRLowerer::lower_stmt(ASTNode* node, std::string& error) {
    if (!node) return true;

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
                if (it == async_site_index_.end()) {
                    error = "await var init has no matching site";
                    return false;
                }
                async_result_places_[it->second] = place;
                return async_lower_await_var_init(vi, it->second, error);
            }
            if (vi->known_type() || (vi->value && vi->value->getType())) {
                BaseType* vtype = vi->known_type();
                if (!vtype && vi->value) vtype = vi->value->getType();
                const MIRTypeRecord& vr = module_.types.get(vt);
                if (!(vr.flags & TF_TYPEDEF) && needs_aggregate_path(module_, vt)) {
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
                        if (!have_ptr) {
                            cur_ptr = builder_->field_addr(cur_place, fc, ft);
                            have_ptr = true;
                        } else {
                            cur_ptr = builder_->field_addr_ptr(cur_ptr, fc, ft);
                        }
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
                        if (is_struct_like(module_, ft)) {
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
                    if (is_struct_like(module_, et)) {
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
                    if (is_struct_like(module_, bt)) {
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
                if (compound && is_struct_like(module_, et)) {
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
            if (place == MIR_INVALID_ID) {
                if (id->linked && id->linked->kind() == ASTNodeKind::VarInitStmt) {
                    auto* vi = id->linked->as_var_init();
                    const SymbolId gs = intern_global(vi);
                    const TypeId vt = types_.map(id->getType());
                    const TypeId ptrt = types_.pointer_type(vt, true);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    if (r.kind != MIRExprKind::Value) {
                        error = "aggregate global assignment not yet supported";
                        return false;
                    }
                    builder_->store_indirect(addr, r.id);
                    return true;
                }
                const std::string gname(id->value.data(), id->value.size());
                if (!gname.empty()) {
                    const SymbolId gs = intern_named_global(gname);
                    const TypeId vt = types_.map(id->getType());
                    const TypeId ptrt = types_.pointer_type(vt, true);
                    const ValueId addr = builder_->global_addr(gs, ptrt);
                    MIRExprResult r = lower_expr(as->value, error);
                    if (!r.ok()) return false;
                    if (r.kind != MIRExprKind::Value) {
                        error = "aggregate global assignment not yet supported";
                        return false;
                    }
                    builder_->store_indirect(addr, r.id);
                    return true;
                }
                error = "assignment target does not resolve to a local place";
                return false;
            }
            const TypeId place_type = builder_->function().places[place].type;
            if (as->assOp != Operation::Assignment && is_struct_like(module_, place_type)) {
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
            if (r.kind == MIRExprKind::Place) {
                destroy_old_dest();
                builder_->emit(MIROpcode::Store, MIR_NULL,
                               {MIROperand::place(place, place_type),
                                MIROperand::place(static_cast<PlaceId>(r.id), place_type)});
                return true;
            }
            if (r.kind == MIRExprKind::Address) {
                destroy_old_dest();
                const ValueId loaded =
                    builder_->load_indirect(static_cast<ValueId>(r.id), place_type);
                builder_->store(place, loaded);
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
        case ASTNodeKind::ValueWrapper: {
            auto* vw = node->as_value_wrapper();
            MIRExprResult r = lower_expr(vw->value, error);
            return r.ok();
        }
        case ASTNodeKind::ValueNode: {
            auto* vn = node->as_value_node_unsafe();
            MIRExprResult r = lower_expr(vn->value, error);
            return r.ok();
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
            return r.ok();
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
            return lower_scope_nodes(b->nodes, error);
        }
        case ASTNodeKind::Scope: {
            auto* s = node->as_scope_unsafe();
            return lower_scope_nodes(s->nodes, error);
        }
        case ASTNodeKind::UnsafeBlock: {
            auto* u = node->as_unsafe_block_unsafe();
            return lower_scope_nodes(u->scope.nodes, error);
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
    if (is_struct_like(module_, vt)) {
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
    return lower_scope_nodes(scope.nodes, error);
}

bool MIRLowerer::lower_scope_nodes(std::vector<ASTNode*>& nodes, std::string& error) {
    const size_t mark = destructibles_.size();
    for (ASTNode* n : nodes) {
        // statements after a terminator (break/continue/return) are unreachable
        if (builder_->current_block_terminated()) break;
        if (!lower_stmt(n, error)) return false;
    }
    // destroy locals declared in this scope (unless they were moved out)
    if (!builder_->current_block_terminated()) {
        for (size_t i = mark; i < destructibles_.size(); ++i) {
            const ValueId fv = builder_->load(destructibles_[i].flag, types_.bool_type());
            builder_->drop(destructibles_[i].place, destructibles_[i].dtor, fv);
        }
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
    const size_t mark = destructibles_.size();
    auto finish = [&]() -> bool {
        if (!builder_->current_block_terminated()) {
            for (size_t i = mark; i < destructibles_.size(); ++i) {
                const ValueId fv = builder_->load(destructibles_[i].flag, types_.bool_type());
                builder_->drop(destructibles_[i].place, destructibles_[i].dtor, fv);
            }
        }
        destructibles_.resize(mark);
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
    if (needs_aggregate_path(module_, vt)) {
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
    if (needs_aggregate_path(module_, vt)) {
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
    if (needs_aggregate_path(module_, vt)) {
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

// ── async / coroutine lowering ─────────────────────────────────────────────

static bool mir_slot_is_destructible(BaseType* type) {
    if (type == nullptr) return false;
    if (type->get_destructor() != nullptr) return true;
    const auto canonical = type->canonical();
    if (canonical->kind() != BaseTypeKind::Array) return false;
    const auto arr = canonical->as_array_type_unsafe();
    if (!arr->has_array_size() || arr->elem_type == nullptr) return false;
    return arr->elem_type->canonical()->get_destructor() != nullptr;
}

std::string MIRLowerer::async_slot_field(unsigned id) const {
    return "__chx_slot_" + std::to_string(id);
}
std::string MIRLowerer::async_child_field(unsigned id) const {
    return "__chx_child_" + std::to_string(id);
}
std::string MIRLowerer::async_drop_flag_field(unsigned id) const {
    return "__chx_drop_" + std::to_string(id);
}
std::string MIRLowerer::async_resident_field(ASTNode* node) const {
    auto it = async_resident_.find(node);
    if (it == async_resident_.end()) return std::string();
    return async_slot_field(it->second);
}
PlaceId MIRLowerer::async_resident_place(ASTNode* node) const {
    auto it = var_places_.find(node);
    return it == var_places_.end() ? MIR_INVALID_ID : it->second;
}
void MIRLowerer::async_emit_spill(const AwaitSite&) {}
void MIRLowerer::async_emit_reload(const AwaitSite&) {}
void MIRLowerer::async_emit_pending_return() {}

static std::string mir_sym_name(const MIRModule& module, SymbolId sym) {
    if (sym == MIR_INVALID_ID || sym >= module.symbols.size()) return std::string();
    const MIRSymbolRecord& r = module.symbols.get(sym);
    return std::string(module.symbols.name_data(r), r.name_length);
}

bool MIRLowerer::lower_async_function(FunctionDeclaration* decl, MIRArena& arena,
                                      MIRFunction& func, std::string& error) {
    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();
    async_ = true;
    arena_ = &arena;
    async_pre_decls_.clear();
    async_post_decls_.clear();
    async_resident_.clear();
    async_drop_flag_slots_.clear();
    async_site_index_.clear();
    async_plan_ = build_async_plan(decl);
    const AsyncCTypes t = resolve_async_c_types(decl);
    if (!t.ok) {
        error = "async lowering could not resolve the core::async protocol types";
        async_ = false;
        return false;
    }
    async_inner_ = t.inner;
    async_handle_ = t.handle;
    async_table_ = t.table;
    async_poll_base_ = t.poll;
    async_context_ = t.context_ptr;

    async_mangled_.clear();
    if (mangler_) async_mangled_ = mangler_(decl);
    if (async_mangled_.empty()) {
        const chem::string_view nv = decl->name_view();
        async_mangled_.assign(nv.data(), nv.size());
    }
    async_frame_type_ = types_.named_struct(async_mangled_ + "__frame");
    async_frame_ptr_type_ = types_.pointer_type(async_frame_type_, true);

    // resident slots (parameters + locals that cross a suspension) and drop flags
    std::vector<bool> crosses(async_plan_.slots.size(), false);
    for (auto& site : async_plan_.sites) {
        for (auto id : site.live_slots) {
            if (id < crosses.size()) crosses[id] = true;
        }
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node == nullptr) continue;
        const bool is_param = slot.node->kind() == ASTNodeKind::FunctionParam;
        if (!crosses[id] && !is_param) continue;
        if (slot.type == nullptr) continue;
        async_resident_[slot.node] = static_cast<unsigned>(id);
    }
    for (auto& site : async_plan_.sites) {
        for (auto id : site.live_drops) async_drop_flag_slots_.insert(id);
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node != nullptr && slot.node->kind() == ASTNodeKind::FunctionParam &&
            async_resident_.count(slot.node) != 0 && slot.destructible) {
            async_drop_flag_slots_.insert(static_cast<unsigned>(id));
        }
    }

    // per-site poll types
    async_site_poll_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        const AsyncCTypes st = resolve_async_c_types_from_handle(
            const_cast<BaseType*>(async_plan_.sites[i].awaited_handle_type));
        async_site_poll_[i] = st.ok ? types_.map(st.poll) : types_.map(async_poll_base_);
    }
    async_site_index_.clear();
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        if (async_plan_.sites[i].var_init != nullptr) {
            async_site_index_[async_plan_.sites[i].var_init] = i;
        }
    }

    async_emit_frame_helpers(decl);
    if (!async_lower_poll_body(decl, error)) { async_ = false; return false; }
    if (!async_lower_ramp(decl, func, error)) { async_ = false; return false; }
    async_ = false;
    return true;
}

void MIRLowerer::async_emit_frame_helpers(FunctionDeclaration* decl) {
    const std::string frame = async_mangled_ + "__frame";
    const std::string inner_c = c_type_of(module_, types_.map(async_inner_));
    const std::string ctx_c = c_type_of(module_, types_.map(async_context_));
    const std::string table_c = c_type_of(module_, types_.map(async_handle_));
    // (the table type is not directly needed; the vtable is spelled below)
    (void)table_c;

    async_pre_decls_ += "struct " + frame + ";\n";
    async_pre_decls_ += "struct " + frame + " { uint32_t __state; " + inner_c + " __result; " +
                        ctx_c + " __cx;";
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        BaseType* ct = async_plan_.sites[i].awaited_handle_type;
        async_pre_decls_ += " " +
                            (ct ? c_type_of(module_, types_.map(ct)) : std::string("void*")) + " " +
                            async_child_field(static_cast<unsigned>(i)) + ";";
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        BaseType* st = async_plan_.slots[id].type;
        const std::string nm = async_slot_field(static_cast<unsigned>(id));
        if (st != nullptr && st->kind() == BaseTypeKind::Array) {
            auto* arr = st->as_array_type_unsafe();
            async_pre_decls_ += " " + c_type_of(module_, types_.map(arr->elem_type)) + " " + nm +
                                "[" + std::to_string(arr->get_array_size()) + "];";
        } else {
            async_pre_decls_ += " " + (st ? c_type_of(module_, types_.map(st))
                                          : std::string("void*")) + " " + nm + ";";
        }
    }
    for (size_t id = 0; id < async_plan_.slots.size(); ++id) {
        if (async_drop_flag_slots_.count(static_cast<unsigned>(id)) != 0) {
            async_pre_decls_ += " uint8_t " + async_drop_flag_field(static_cast<unsigned>(id)) +
                                ";";
        }
    }
    async_pre_decls_ += " };\n";

    const std::string poll_ptr = c_type_of(module_, types_.pointer_type(types_.map(async_poll_base_), true));
    const std::string handle_ptr = c_type_of(module_, types_.pointer_type(types_.map(async_handle_), true));
    async_pre_decls_ += "static void " + async_mangled_ + "__poll(" + poll_ptr +
                        " __chx__async_ret, void* __frame, " + ctx_c + " __cx);\n";
    async_pre_decls_ += "static void " + async_mangled_ + "__drop(void* __frame);\n";

    // drop definition
    async_post_decls_ += "static void " + async_mangled_ + "__drop(void* __frame) {\n";
    async_post_decls_ += "    struct " + frame + "* __chx__af = (struct " + frame +
                         "*) __frame;\n";
    async_post_decls_ += "    switch(__chx__af->__state) {\n";
    for (size_t i = 0; i < async_plan_.sites.size(); ++i) {
        async_post_decls_ += "        case " + std::to_string(i + 1) + ":\n";
        for (auto id : async_plan_.sites[i].live_drops) {
            const auto& slot = async_plan_.slots[id];
            if (slot.type == nullptr) continue;
            SymbolId dtor = destructor_symbol(slot.type);
            std::string dname = mir_sym_name(module_, dtor);
            if (dname.empty()) continue;
            const std::string field = "__chx__af->" + async_slot_field(id);
            if (async_drop_flag_slots_.count(id) != 0) {
                async_post_decls_ += "            if(__chx__af->" + async_drop_flag_field(id) +
                                     ") { " + dname + "(&" + field + "); }\n";
            } else {
                async_post_decls_ += "            " + dname + "(&" + field + ");\n";
            }
        }
        const std::string child = "__chx__af->" + async_child_field(static_cast<unsigned>(i));
        async_post_decls_ += "            if(" + child + ".frame != (void*) 0) { " + child +
                             ".vtbl->drop(" + child + ".frame); " + child +
                             ".frame = (void*) 0; }\n";
        async_post_decls_ += "            break;\n";
    }
    async_post_decls_ += "        default: break;\n    }\n";
    async_post_decls_ += "    chemical_async_frame_free(__frame, sizeof(struct " + frame +
                         "), _Alignof(struct " + frame + "));\n}\n";
    // vtable definition (references __poll and __drop, both defined above)
    const std::string table_t = c_type_of(module_, types_.map(async_table_));
    async_post_decls_ += "static " + table_t + " " + async_mangled_ + "__vtbl = (" + table_t +
                         "){ .poll = " + async_mangled_ + "__poll, .drop = " + async_mangled_ +
                         "__drop };\n";
    (void)handle_ptr;
}

bool MIRLowerer::async_lower_await_var_init(VarInitStatement* stmt, size_t site_index,
                                            std::string& error) {
    if (site_index >= async_plan_.sites.size()) {
        error = "async await has no matching site";
        return false;
    }
    const AwaitSite& site = async_plan_.sites[site_index];
    auto* await = stmt->value ? stmt->value->as_await_expression_unsafe() : nullptr;
    if (await == nullptr || await->getInner() == nullptr) {
        error = "async await has no operand";
        return false;
    }
    // The child future lives in a frame field so it survives suspension.
    const std::string child_name = async_child_field(static_cast<unsigned>(site_index));
    const ConstantId child_fc = module_.constants.add_string(MIR_INVALID_ID, child_name.data(),
                                                             static_cast<uint32_t>(child_name.size()));
    PlaceId child_place = async_child_places_[site_index];
    if (child_place == MIR_INVALID_ID) {
        child_place = builder_->alloca_frame(types_.map(site.awaited_handle_type), child_fc);
        async_child_places_[site_index] = child_place;
    }
    // result place (the awaited value)
    PlaceId result_place = async_result_places_[site_index];
    // lower the operand and store it in the child slot
    MIRExprResult child = lower_expr(await->getInner(), error);
    if (!child.ok()) return false;
    if (child.kind == MIRExprKind::Place) {
        const PlaceId cp = static_cast<PlaceId>(child.id);
        const TypeId cpt = builder_->function().places[cp].type;
        builder_->field_store_ptr(async_poll_af_val_, child_fc, builder_->load(cp, cpt));
        // the child is consumed by the await; don't destroy the source again
        mark_moved(cp);
    } else {
        builder_->field_store_ptr(async_poll_af_val_, child_fc,
                                  static_cast<ValueId>(child.id));
    }
    // poll
    std::vector<MIROperand> ops;
    ops.push_back(MIROperand::place(async_poll_frame_, async_frame_ptr_type_));
    ops.push_back(MIROperand::place(child_place, types_.map(site.awaited_handle_type)));
    ops.push_back(MIROperand::place(result_place, types_.map(site.awaited_type)));
    ops.push_back(MIROperand::type(async_site_poll_[site_index]));
    ops.push_back(MIROperand::constant(
        module_.constants.add_int(MIR_INVALID_ID, site_index + 1), MIR_INVALID_ID));
    ops.push_back(MIROperand::place(async_poll_ret_, types_.pointer_type(types_.map(async_poll_base_), true)));
    ops.push_back(MIROperand::type(types_.map(async_poll_base_)));
    // spills
    std::vector<std::pair<PlaceId, ConstantId>> spills;
    for (auto id : site.live_slots) {
        const auto& slot = async_plan_.slots[id];
        if (slot.node == site.var_init) continue;
        if (async_resident_.count(slot.node) != 0) continue;
        PlaceId lp = place_for_linked(slot.node);
        if (lp == MIR_INVALID_ID) continue;
        const std::string fname = async_slot_field(id);
        const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, fname.data(),
                                                           static_cast<uint32_t>(fname.size()));
        spills.emplace_back(lp, fc);
    }
    async_spills_[site_index] = spills;
    ops.push_back(MIROperand::constant(
        module_.constants.add_int(MIR_INVALID_ID, spills.size()), MIR_INVALID_ID));
    for (auto& sp : spills) {
        ops.push_back(MIROperand::place(sp.first, MIR_INVALID_ID));
        ops.push_back(MIROperand::constant(sp.second, MIR_INVALID_ID));
    }
    builder_->emit(MIROpcode::AsyncAwait, MIR_NULL, ops.data(), static_cast<uint32_t>(ops.size()));
    // the continuation after the await is reached from both the first pass and
    // the resume block, so give it its own block
    const BlockId cont = builder_->create_block();
    builder_->br(cont);
    builder_->set_block(cont);
    async_cont_blocks_[site_index] = cont;
    return true;
}

bool MIRLowerer::async_lower_poll_body(FunctionDeclaration* decl, std::string& error) {
    MIRFunction& fn = async_poll_;
    fn.clear();
    const std::string poll_name = async_mangled_ + "__poll";
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    SymbolId sym = module_.symbols.add(rec, poll_name.data(),
                                       static_cast<uint32_t>(poll_name.size()), poll_name.data(),
                                       static_cast<uint32_t>(poll_name.size()));
    fn.symbol = sym;
    const TypeId poll_ptr = types_.pointer_type(types_.map(async_poll_base_), true);
    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    const TypeId ctx_ptr = types_.map(async_context_);
    std::vector<TypeId> pts{poll_ptr, void_ptr, ctx_ptr};
    const TypeId ftype = types_.function_signature(types_.void_type(), pts);
    module_.symbols.symbols[sym].type = ftype;
    fn.function_type = ftype;

    MIRBuilder b(*arena_, module_, fn);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    ValueId p0 = b.param(poll_ptr);
    PlaceId ret_place = b.alloca(poll_ptr, MIRStorageClass::Parameter);
    b.store(ret_place, p0);
    ValueId p1 = b.param(void_ptr);
    PlaceId frame_arg = b.alloca(void_ptr, MIRStorageClass::Parameter);
    b.store(frame_arg, p1);
    ValueId p2 = b.param(ctx_ptr);
    PlaceId cx_place = b.alloca(ctx_ptr, MIRStorageClass::Parameter);
    b.store(cx_place, p2);
    async_poll_ret_ = ret_place;
    async_poll_cx_ = cx_place;

    ValueId fv = b.load(frame_arg, void_ptr);
    ValueId af = b.cast(fv, async_frame_ptr_type_);
    PlaceId af_place = b.alloca(async_frame_ptr_type_, MIRStorageClass::FramePtr);
    b.store(af_place, af);
    async_poll_frame_ = af_place;
    async_poll_af_val_ = af;
    // stash the context in the frame so child polls can reach the waker
    {
        const ConstantId cx_fc = module_.constants.add_string(MIR_INVALID_ID, "__cx", 4);
        ValueId cxv = b.load(cx_place, ctx_ptr);
        b.field_store_ptr(af, cx_fc, cxv);
    }

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();
    async_child_places_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    async_result_places_.assign(async_plan_.sites.size(), MIR_INVALID_ID);
    async_spills_.assign(async_plan_.sites.size(), {});
    async_cont_blocks_.assign(async_plan_.sites.size(), MIR_INVALID_ID);

    const size_t nsites = async_plan_.sites.size();
    const BlockId l0_b = b.create_block();
    std::vector<BlockId> resume_b(nsites);
    for (size_t i = 0; i < nsites; ++i) resume_b[i] = b.create_block();
    const BlockId default_b = b.create_block();
    std::vector<BlockId> chain(nsites + 1);
    for (size_t i = 0; i <= nsites; ++i) chain[i] = b.create_block();

    const TypeId u32 = types_.u32_type();
    const ConstantId state_fc = module_.constants.add_string(MIR_INVALID_ID, "__state", 7);
    ValueId st = b.field_load_ptr(af, state_fc, u32);
    const ConstantId eqc = module_.constants.add_int(MIR_INVALID_ID,
                                                     static_cast<uint64_t>(MIRBinaryOp::Eq));
    for (size_t i = 0; i <= nsites; ++i) {
        const ValueId cval = b.const_int(u32, module_.constants.add_int(u32, i));
        ValueId iseq = b.compare(st, cval, eqc, types_.bool_type());
        const BlockId target = (i == 0) ? l0_b : resume_b[i - 1];
        b.cond_br(iseq, target, chain[i]);
        b.set_block(chain[i]);
    }
    b.br(default_b);

    // default: Ready(af->__result)
    b.set_block(default_b);
    b.emit(MIROpcode::AsyncFinish, MIR_NULL,
           {MIROperand::place(af_place, async_frame_ptr_type_),
            MIROperand::place(ret_place, poll_ptr),
            MIROperand::type(types_.map(async_poll_base_))});

    // L0: body
    b.set_block(l0_b);
    for (FunctionParam* p : decl->params) {
        // the frame stores the declared type (aggregates by value), so the poll
        // parameter is the declared type too (not the hidden pointer form)
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        const std::string rf = async_resident_field(p);
        if (!rf.empty()) {
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, rf.data(),
                                                               static_cast<uint32_t>(rf.size()));
            PlaceId place = b.alloca_frame(pt, fc);
            bind(p, place);
            bind_name(p->name, place);
        } else {
            PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
            // load the spilled value from the frame slot
            auto it = std::find_if(async_plan_.slots.begin(), async_plan_.slots.end(),
                                   [&](const AsyncFrameSlot& s) { return s.node == p; });
            if (it != async_plan_.slots.end()) {
                const unsigned sid = static_cast<unsigned>(it - async_plan_.slots.begin());
                const std::string sf = async_slot_field(sid);
                const ConstantId fc = module_.constants.add_string(
                    MIR_INVALID_ID, sf.data(), static_cast<uint32_t>(sf.size()));
                ValueId sv = b.field_load_ptr(af, fc, pt);
                b.store(place, sv);
            }
            bind(p, place);
            bind_name(p->name, place);
        }
    }
    if (decl->body.has_value()) {
        if (!lower_scope_nodes(decl->body->nodes, error)) {
            builder_ = nullptr;
            return false;
        }
    }
    if (!b.current_block_terminated()) {
        emit_drops();
        b.emit(MIROpcode::AsyncFinish, MIR_NULL,
               {MIROperand::place(af_place, async_frame_ptr_type_),
                MIROperand::place(ret_place, poll_ptr),
                MIROperand::type(types_.map(async_poll_base_))});
    }

    // resume blocks
    for (size_t i = 0; i < nsites; ++i) {
        b.set_block(resume_b[i]);
        const AwaitSite& site = async_plan_.sites[i];
        // reload spilled slots
        for (auto& sp : async_spills_[i]) {
            // sp.first is the local place; the frame field name is the constant
            const MIRConstant& c = module_.constants.get(sp.second);
            std::string fname(module_.constants.data.data() + c.data_offset, c.data_count);
            const TypeId lpt = builder_->function().places[sp.first].type;
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, fname.data(),
                                                               static_cast<uint32_t>(fname.size()));
            ValueId sv = b.field_load_ptr(async_poll_af_val_, fc, lpt);
            b.store(sp.first, sv);
        }
        // AsyncAwait with resume_state = 0
        std::vector<MIROperand> ops;
        ops.push_back(MIROperand::place(async_poll_frame_, async_frame_ptr_type_));
        ops.push_back(MIROperand::place(async_child_places_[i],
                                        types_.map(site.awaited_handle_type)));
        ops.push_back(MIROperand::place(async_result_places_[i], types_.map(site.awaited_type)));
        ops.push_back(MIROperand::type(async_site_poll_[i]));
        ops.push_back(MIROperand::constant(module_.constants.add_int(MIR_INVALID_ID, 0),
                                           MIR_INVALID_ID));
        ops.push_back(MIROperand::place(async_poll_ret_,
                                        types_.pointer_type(types_.map(async_poll_base_), true)));
        ops.push_back(MIROperand::type(types_.map(async_poll_base_)));
        ops.push_back(MIROperand::constant(
            module_.constants.add_int(MIR_INVALID_ID, async_spills_[i].size()), MIR_INVALID_ID));
        for (auto& sp : async_spills_[i]) {
            ops.push_back(MIROperand::place(sp.first, MIR_INVALID_ID));
            ops.push_back(MIROperand::constant(sp.second, MIR_INVALID_ID));
        }
        b.emit(MIROpcode::AsyncAwait, MIR_NULL, ops.data(), static_cast<uint32_t>(ops.size()));
        const BlockId cont = async_cont_blocks_[i];
        b.br(cont != MIR_INVALID_ID ? cont : default_b);
    }

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "async poll builder failure";
        return false;
    }
    return true;
}

bool MIRLowerer::async_lower_ramp(FunctionDeclaration* decl, MIRFunction& func,
                                  std::string& error) {
    // The ramp returns the handle; for the eager path it runs the body inline.
    const bool eager = async_plan_.sites.empty();
    const TypeId handle_t = types_.map(async_handle_);
    sret_ = true;
    sret_ret_type_ = handle_t;
    sret_ptr_type_ = types_.pointer_type(handle_t, true);
    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
    bool self_in_params = false;
    for (FunctionParam* p : decl->params) {
        if (p == self_param) { self_in_params = true; break; }
    }
    if (self_in_params) self_param = nullptr;
    {
        std::vector<TypeId> ptypes;
        ptypes.push_back(sret_ptr_type_);
        if (self_param) {
            TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
                pt = types_.pointer_type(module_.types.get(pt).element, true);
            } else if (needs_aggregate_path(module_, pt)) {
                pt = types_.pointer_type(pt, true);
            }
            ptypes.push_back(pt);
        }
        for (FunctionParam* p : decl->params) {
            TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
                pt = types_.pointer_type(module_.types.get(pt).element, true);
            } else if (needs_aggregate_path(module_, pt)) {
                pt = types_.pointer_type(pt, true);
            }
            ptypes.push_back(pt);
        }
        const TypeId ftype = types_.function_signature(types_.void_type(), ptypes);
        func.symbol = intern_function(decl);
        module_.symbols.symbols[func.symbol].type = ftype;
        func.function_type = ftype;
    }

    MIRBuilder b(*arena_, module_, func);
    builder_ = &b;
    const BlockId entry = b.create_block();
    b.set_block(entry);
    func.entry_block = entry;
    sret_ptr_ = b.param(sret_ptr_type_);
    const PlaceId sret_place = b.alloca(sret_ptr_type_, MIRStorageClass::Parameter);
    b.store(sret_place, sret_ptr_);

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();

    // frame alloc
    ValueId af = b.async_frame_alloc(async_frame_type_, async_frame_ptr_type_);
    PlaceId af_place = b.alloca(async_frame_ptr_type_, MIRStorageClass::FramePtr);
    b.store(af_place, af);
    async_ramp_af_ = af_place;
    async_ramp_af_val_ = af;
    // set state = 0 and cx = 0
    const ConstantId st_fc = module_.constants.add_string(MIR_INVALID_ID, "__state", 7);
    const ConstantId cx_fc = module_.constants.add_string(MIR_INVALID_ID, "__cx", 4);
    ValueId zero = b.const_int(types_.u32_type(), module_.constants.add_int(types_.u32_type(), 0));
    b.field_store_ptr(af, st_fc, zero);
    ValueId nullv = b.const_null(types_.map(async_context_),
                                 module_.constants.add_int(types_.map(async_context_), 0));
    b.field_store_ptr(af, cx_fc, nullv);

    // bind self/params and spill parameters into the frame
    if (self_param) {
        TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(self_param, place);
        bind_name(self_param->name, place);
    }
    for (FunctionParam* p : decl->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(p, place);
        bind_name(p->name, place);
        // store the parameter into its frame slot
        auto it = std::find_if(async_plan_.slots.begin(), async_plan_.slots.end(),
                               [&](const AsyncFrameSlot& s) { return s.node == p; });
        if (it != async_plan_.slots.end()) {
            const unsigned sid = static_cast<unsigned>(it - async_plan_.slots.begin());
            const std::string sf = async_slot_field(sid);
            const ConstantId fc = module_.constants.add_string(MIR_INVALID_ID, sf.data(),
                                                               static_cast<uint32_t>(sf.size()));
            const MIRTypeRecord& pr = module_.types.get(pt);
            ValueId src = b.load(place, pt);
            if (pr.kind == MIRTypeKind::Pointer || pr.kind == MIRTypeKind::Reference) {
                // aggregate parameter arrives as a hidden pointer: copy through it
                src = b.load_indirect(src, types_.map(p->type));
            }
            b.field_store_ptr(af, fc, src);
            if (async_drop_flag_slots_.count(sid) != 0) {
                const std::string df = async_drop_flag_field(sid);
                const ConstantId dfc = module_.constants.add_string(MIR_INVALID_ID, df.data(),
                                                                    static_cast<uint32_t>(df.size()));
                ValueId one = b.const_int(types_.u32_type(),
                                          module_.constants.add_int(types_.u32_type(), 1));
                b.field_store_ptr(af, dfc, one);
            }
        }
    }

    if (eager) {
        // run the body inline, redirecting `return e` into the frame result
        async_in_ramp_ = true;
        async_ramp_result_ = MIR_INVALID_ID;
        async_ramp_done_ = b.create_block();
        if (decl->body.has_value()) {
            if (!lower_scope_nodes(decl->body->nodes, error)) {
                builder_ = nullptr;
                return false;
            }
        }
        if (!b.current_block_terminated()) b.br(async_ramp_done_);
        b.set_block(async_ramp_done_);
        async_in_ramp_ = false;
    }
    // return the handle
    const ConstantId vtbl_c = module_.constants.add_string(
        MIR_INVALID_ID, (async_mangled_ + "__vtbl").data(),
        static_cast<uint32_t>((async_mangled_ + "__vtbl").size()));
    ValueId state_one = b.const_int(types_.u32_type(),
                                    module_.constants.add_int(types_.u32_type(), eager ? 1 : 0));
    (void)state_one;
    const ConstantId ramp_state_c =
        module_.constants.add_int(types_.u32_type(), eager ? 1 : 0);
    b.emit(MIROpcode::AsyncRampFinish, MIR_NULL,
           {MIROperand::place(af_place, async_frame_ptr_type_),
            MIROperand::place(sret_place, sret_ptr_type_),
            MIROperand::constant(ramp_state_c, MIR_INVALID_ID),
            MIROperand::constant(vtbl_c, MIR_INVALID_ID),
            MIROperand::type(handle_t)});

    builder_ = nullptr;
    if (!b.ok()) {
        error = b.error() ? b.error() : "async ramp builder failure";
        return false;
    }
    return true;
}

MIRExprResult MIRLowerer::lower_captured_variable(CapturedVariable* cap, bool as_address,
                                                  std::string& error) {
    if (lambda_closure_ptr_ == MIR_NULL) {
        error = "captured variable used outside its lambda";
        return MIRExprResult::error();
    }
    const chem::string_view nm = cap->name;
    const ConstantId fc = module_.constants.add_string(
        MIR_INVALID_ID, nm.data(), static_cast<uint32_t>(nm.size()));
    BaseType* ct = cap->known_type();
    const TypeId ft = types_.map(ct);
    const ValueId ptr = lambda_closure_ptr_;
    if (cap->capture_by_ref) {
        // the field is a pointer to the referent; the captured variable is that
        // reference, so its value is the pointer itself
        return MIRExprResult::value(builder_->field_load_ptr(ptr, fc, ft), ft);
    }
    if (as_address || needs_aggregate_path(module_, ft)) {
        return MIRExprResult::address(builder_->field_addr_ptr(ptr, fc, ft), ft);
    }
    return MIRExprResult::value(builder_->field_load_ptr(ptr, fc, ft), ft);
}

MIRExprResult MIRLowerer::lower_extraction_value(ExtractionValue* ev, std::string& error) {
    Value* src = ev ? ev->value : nullptr;
    if (!src || src->val_kind() != ValueKind::LambdaFunc) {
        error = "extraction value source must be a lambda";
        return MIRExprResult::error();
    }
    auto* lamb = src->as_lambda_func_unsafe();
    if (!materialize_capturing_lambda(lamb, error)) return MIRExprResult::error();
    const LambdaMaterialization& m = lambda_materializations_[lamb];
    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    switch (ev->extractionKind) {
        case ExtractionKind::LambdaFnPtr:
            return MIRExprResult::value(m.fn_ptr, m.fn_ptr_type);
        case ExtractionKind::LambdaCapturedPtr: {
            if (m.cap_temp == MIR_INVALID_ID) return MIRExprResult::value(
                builder_->const_null(void_ptr, module_.constants.add_null(void_ptr)), void_ptr);
            const TypeId ptrt = types_.pointer_type(m.cap_type, true);
            return MIRExprResult::value(builder_->address_of(m.cap_temp, ptrt), ptrt);
        }
        case ExtractionKind::LambdaCapturedDestructor:
            if (m.dtor_ptr == MIR_NULL) {
                return MIRExprResult::value(
                    builder_->const_null(void_ptr, module_.constants.add_null(void_ptr)), void_ptr);
            }
            return MIRExprResult::value(m.dtor_ptr, m.dtor_type);
        case ExtractionKind::SizeOfLambdaCaptured:
            return MIRExprResult::value(builder_->size_of(m.cap_type, types_.u64_type()),
                                        types_.u64_type());
        case ExtractionKind::AlignOfLambdaCaptured:
            return MIRExprResult::value(builder_->align_of(m.cap_type, types_.u64_type()),
                                        types_.u64_type());
        default:
            error = "unsupported extraction value kind";
            return MIRExprResult::error();
    }
}

MIRExprResult MIRLowerer::lower_capturing_function_call(MIRExprResult recv, BaseType* recv_type,
                                                        FunctionCall* call, std::string& error) {
    if (!recv.ok()) return recv;
    BaseType* cc = recv_type ? recv_type->canonical() : nullptr;
    CapturingFunctionType* cap = cc ? cc->get_cap_func_type() : nullptr;
    BaseType* inst = cap ? const_cast<BaseType*>(cap->instance_type.getType()) : nullptr;
    if (!inst && cc) {
        // `std::function<T>` may resolve to the instance struct directly
        ASTNode* cln = cc->get_direct_linked_canonical_node();
        if (cln && cln->kind() == ASTNodeKind::StructDecl) {
            inst = cln->as_struct_def_unsafe()->known_type();
        }
    }
    if (!inst) {
        error = "capturing function instance could not be resolved";
        return MIRExprResult::error();
    }
    FunctionDeclaration* get_fn = resolve_method_fallback(inst, "get_fn_ptr");
    FunctionDeclaration* get_data = resolve_method_fallback(inst, "get_data_ptr");
    if (!get_fn || !get_data) {
        error = "capturing function instance methods not found";
        return MIRExprResult::error();
    }
    auto call_method = [&](FunctionDeclaration* m) -> MIRExprResult {
        VariableIdentifier id(chem::string_view(), nullptr, ZERO_LOC);
        id.linked = m;
        FunctionCall sc(&id, m->returnType, ZERO_LOC);
        return lower_method_call(recv, cc, &sc, error);
    };
    MIRExprResult fnr = call_method(get_fn);
    if (!fnr.ok()) return fnr;
    MIRExprResult dr = call_method(get_data);
    if (!dr.ok()) return dr;
    const TypeId vpt = types_.pointer_type(types_.void_type(), true);
    const ValueId fnptr = (fnr.kind == MIRExprKind::Place)
                              ? builder_->load(static_cast<PlaceId>(fnr.id), fnr.type)
                              : static_cast<ValueId>(fnr.id);
    const ValueId dataptr = (dr.kind == MIRExprKind::Place)
                                ? builder_->load(static_cast<PlaceId>(dr.id), dr.type)
                                : static_cast<ValueId>(dr.id);
    std::vector<MIROperand> cargs;
    cargs.push_back(MIROperand::value(dataptr, vpt));
    const size_t ccap = call_temps_.size();
    if (!lower_call_args_for(nullptr, false, call->values, cargs, error)) {
        return MIRExprResult::error();
    }
    const TypeId crt = types_.map(call->getType());
    std::vector<TypeId> cptypes;
    cptypes.reserve(cargs.size());
    for (auto& op : cargs) cptypes.push_back(op.type());
    const TypeId cftype = types_.function_signature(crt, cptypes);
    const ValueId cfn = builder_->cast(fnptr, cftype);
    if (is_void_type(module_, crt)) {
        builder_->call_indirect(cfn, crt, cargs.data(), static_cast<uint32_t>(cargs.size()));
        destroy_call_temps_from(ccap, error);
        return MIRExprResult::void_result();
    }
    if (needs_sret_return(module_, crt)) {
        const PlaceId tmp = builder_->alloca(crt, MIRStorageClass::Temporary);
        builder_->zero_init(tmp);
        builder_->call_indirect_sret(cfn, tmp, cargs.data(), static_cast<uint32_t>(cargs.size()));
        destroy_call_temps_from(ccap, error);
        return MIRExprResult::place(tmp, crt);
    }
    const ValueId cv =
        builder_->call_indirect(cfn, crt, cargs.data(), static_cast<uint32_t>(cargs.size()));
    destroy_call_temps_from(ccap, error);
    return MIRExprResult::value(cv, crt);
}

bool MIRLowerer::materialize_capturing_lambda(LambdaFunction* lamb, std::string& error) {
    if (lambda_materializations_.find(lamb) != lambda_materializations_.end()) return true;
    if (!lamb || lamb->captureList.empty()) {
        error = "materialize_capturing_lambda on a non-capturing lambda";
        return false;
    }

    static uint64_t s_counter = 0;
    const std::string lname = "__chemda_cap_" + std::to_string(s_counter++);
    const std::string cap_name = lname + "_cap";

    // 1. the closure struct definition + type
    const TypeId cap_type = types_.named_struct(cap_name);
    std::string sdef = "struct " + cap_name + " {\n";
    for (CapturedVariable* cap : lamb->captureList) {
        const TypeId ft = types_.map(cap->known_type());
        sdef += "    " + c_type_of(module_, ft) + " " +
                std::string(cap->name.data(), cap->name.size()) + ";\n";
    }
    sdef += "};\n";
    lambda_defs_.push_back(sdef);

    // 2. the closure destructor
    const std::string dname = cap_name + "_destr";
    const TypeId void_ptr = types_.pointer_type(types_.void_type(), true);
    MIRSymbolRecord drec;
    drec.kind = MIRSymbolKind::Function;
    drec.linkage = MIRLinkage::Internal;
    drec.type = types_.function_signature(types_.void_type(), {void_ptr});
    const SymbolId dtor_sym = module_.symbols.add(
        drec, dname.data(), static_cast<uint32_t>(dname.size()), dname.data(),
        static_cast<uint32_t>(dname.size()));
    std::string ddef = "static void " + dname + "(struct " + cap_name + "* self) {\n";
    if (lamb->has_destructor_for_capture()) {
        for (CapturedVariable* cap : lamb->captureList) {
            const SymbolId d = destructor_symbol(cap->known_type());
            if (d == MIR_INVALID_ID) continue;
            const MIRSymbolRecord& dr = module_.symbols.get(d);
            const std::string dn(module_.symbols.mangled_data(dr), dr.mangled_length);
            const std::string fname(cap->name.data(), cap->name.size());
            ddef += "    " + dn + "(&self->" + fname + ");\n";
        }
    }
    ddef += "}\n";
    lambda_defs_.push_back(ddef);

    // 3. the lambda function: `ret <lname>(void* this, <params>)`
    const TypeId ret = lamb->returnType ? types_.map(lamb->returnType) : types_.void_type();
    const bool l_sret = needs_sret_return(module_, ret);
    std::vector<TypeId> ptypes;
    if (l_sret) ptypes.push_back(types_.pointer_type(ret, true));
    ptypes.push_back(void_ptr);
    for (FunctionParam* p : lamb->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ptypes.push_back(pt);
    }
    const TypeId ftype = types_.function_signature(l_sret ? types_.void_type() : ret, ptypes);
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    rec.type = ftype;
    const SymbolId sym = module_.symbols.add(
        rec, lname.data(), static_cast<uint32_t>(lname.size()), lname.data(),
        static_cast<uint32_t>(lname.size()));

    MIRFunction fn;
    fn.symbol = sym;
    fn.function_type = ftype;

    // save the enclosing function's lowering state
    MIRBuilder* saved_builder = builder_;
    MIRArena* saved_arena = arena_;
    FunctionTypeBody* saved_cur = current_func_;
    auto saved_var = std::move(var_places_);
    auto saved_name = std::move(name_places_);
    auto saved_destructibles = std::move(destructibles_);
    auto saved_call_temps = std::move(call_temps_);
    auto saved_pattern_bases = std::move(pattern_bases_);
    auto saved_pattern_results = std::move(pattern_results_);
    const bool saved_sret = sret_;
    const ValueId saved_sret_ptr = sret_ptr_;
    const TypeId saved_sret_ret = sret_ret_type_;
    const TypeId saved_sret_ptr_t = sret_ptr_type_;
    BaseType* saved_return_type = return_type_;
    auto saved_break = std::move(break_targets_);
    auto saved_continue = std::move(continue_targets_);
    const ValueId saved_closure = lambda_closure_ptr_;
    const TypeId saved_closure_t = lambda_closure_type_;

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();
    break_targets_.clear();
    continue_targets_.clear();

    MIRBuilder b(*saved_arena, module_, fn);
    builder_ = &b;
    current_func_ = lamb;
    if (set_current_func_) set_current_func_(lamb);
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    sret_ = l_sret;
    sret_ret_type_ = ret;
    return_type_ = lamb->returnType;
    if (l_sret) {
        sret_ptr_type_ = types_.pointer_type(ret, true);
        sret_ptr_ = b.param(sret_ptr_type_);
    }

    bool ok = true;
    const ValueId this_v = b.param(void_ptr);
    const TypeId cap_ptr_t = types_.pointer_type(cap_type, true);
    lambda_closure_ptr_ = b.cast(this_v, cap_ptr_t);
    lambda_closure_type_ = cap_type;

    for (FunctionParam* p : lamb->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        if (place == MIR_INVALID_ID) {
            ok = false;
            error = "failed to allocate lambda parameter place";
            break;
        }
        b.store(place, pv);
        bind(p, place);
        bind_name(p->name, place);
        if (p->type) {
            const TypeId ptt = types_.map(p->type);
            const MIRTypeRecord& prr = module_.types.get(ptt);
            if (!(prr.flags & TF_TYPEDEF) && needs_aggregate_path(module_, ptt)) {
                register_destructible(place, p->type);
            }
        }
    }

    if (ok) ok = lower_scope(lamb->scope, error);
    if (ok && !b.current_block_terminated()) {
        emit_drops();
        b.ret_void();
    }
    if (ok && !b.ok()) {
        ok = false;
        error = b.error() ? b.error() : "MIR builder failure";
    }

    std::string c;
    if (ok) {
        std::string e;
        if (!emit_function_c(fn, module_, c, e)) {
            ok = false;
            error = e;
        }
    }
    if (ok) lambda_defs_.push_back(std::move(c));

    // restore the enclosing function's state
    builder_ = saved_builder;
    arena_ = saved_arena;
    current_func_ = saved_cur;
    if (set_current_func_) set_current_func_(saved_cur);
    var_places_ = std::move(saved_var);
    name_places_ = std::move(saved_name);
    destructibles_ = std::move(saved_destructibles);
    call_temps_ = std::move(saved_call_temps);
    pattern_bases_ = std::move(saved_pattern_bases);
    pattern_results_ = std::move(saved_pattern_results);
    sret_ = saved_sret;
    sret_ptr_ = saved_sret_ptr;
    sret_ret_type_ = saved_sret_ret;
    sret_ptr_type_ = saved_sret_ptr_t;
    return_type_ = saved_return_type;
    break_targets_ = std::move(saved_break);
    continue_targets_ = std::move(saved_continue);
    lambda_closure_ptr_ = saved_closure;
    lambda_closure_type_ = saved_closure_t;

    if (!ok) return false;

    // 4. materialize the closure temp in the enclosing function
    const PlaceId cap_temp = builder_->alloca(cap_type, MIRStorageClass::Temporary);
    if (cap_temp == MIR_INVALID_ID) {
        error = "failed to allocate lambda closure";
        return false;
    }
    builder_->zero_init(cap_temp);
    for (CapturedVariable* cap : lamb->captureList) {
        const ConstantId fc = module_.constants.add_string(
            MIR_INVALID_ID, cap->name.data(), static_cast<uint32_t>(cap->name.size()));
        PlaceId lp = place_for_linked(cap->linked);
        if (lp == MIR_INVALID_ID) lp = place_for_name(cap->name);
        if (lp == MIR_INVALID_ID) {
            error = "captured variable does not resolve to a local place";
            return false;
        }
        const TypeId lt = builder_->function().places[lp].type;
        if (cap->capture_by_ref) {
            const MIRTypeRecord& lr = module_.types.get(lt);
            if (lr.kind == MIRTypeKind::Reference || lr.kind == MIRTypeKind::Pointer) {
                // the captured variable is already a reference: store the pointer
                builder_->field_store(cap_temp, fc, builder_->load(lp, lt));
            } else {
                const TypeId ptrt = types_.pointer_type(lt, true);
                builder_->field_store(cap_temp, fc, builder_->address_of(lp, ptrt));
            }
        } else if (needs_aggregate_path(module_, lt)) {
            builder_->field_store_place(cap_temp, fc, lp);
            mark_moved(lp);
        } else {
            builder_->field_store(cap_temp, fc, builder_->load(lp, lt));
        }
    }

    LambdaMaterialization m;
    m.cap_type = cap_type;
    m.cap_temp = cap_temp;
    m.fn_ptr_type = ftype;
    m.fn_ptr = builder_->function_addr(sym, m.fn_ptr_type);
    m.dtor_type = types_.function_signature(types_.void_type(), {void_ptr});
    m.dtor_ptr = builder_->function_addr(dtor_sym, m.dtor_type);
    lambda_materializations_[lamb] = m;
    return true;
}

MIRExprResult MIRLowerer::lower_lambda(LambdaFunction* lamb, std::string& error) {
    if (!lamb) {
        error = "null lambda";
        return MIRExprResult::error();
    }
    if (!lamb->captureList.empty()) {
        if (!materialize_capturing_lambda(lamb, error)) return MIRExprResult::error();
        const LambdaMaterialization& m = lambda_materializations_[lamb];
        return MIRExprResult::value(m.fn_ptr, m.fn_ptr_type);
    }
    static uint64_t s_lambda_counter = 0;
    const std::string lname = "__chemda_" + std::to_string(s_lambda_counter++);

    const TypeId ret = lamb->returnType ? types_.map(lamb->returnType) : types_.void_type();
    const bool l_sret = needs_sret_return(module_, ret);

    std::vector<TypeId> ptypes;
    if (l_sret) ptypes.push_back(types_.pointer_type(ret, true));
    for (FunctionParam* p : lamb->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ptypes.push_back(pt);
    }
    const TypeId ftype = types_.function_signature(l_sret ? types_.void_type() : ret, ptypes);

    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.linkage = MIRLinkage::Internal;
    rec.type = ftype;
    const SymbolId sym = module_.symbols.add(
        rec, lname.data(), static_cast<uint32_t>(lname.size()),
        lname.data(), static_cast<uint32_t>(lname.size()));

    MIRFunction fn;
    fn.symbol = sym;
    fn.function_type = ftype;

    // save the enclosing function's lowering state
    MIRBuilder* saved_builder = builder_;
    MIRArena* saved_arena = arena_;
    FunctionTypeBody* saved_cur = current_func_;
    auto saved_var = std::move(var_places_);
    auto saved_name = std::move(name_places_);
    auto saved_destructibles = std::move(destructibles_);
    auto saved_call_temps = std::move(call_temps_);
    const bool saved_sret = sret_;
    const ValueId saved_sret_ptr = sret_ptr_;
    const TypeId saved_sret_ret = sret_ret_type_;
    const TypeId saved_sret_ptr_t = sret_ptr_type_;
    BaseType* saved_return_type = return_type_;
    auto saved_break = std::move(break_targets_);
    auto saved_continue = std::move(continue_targets_);

    var_places_.clear();
    name_places_.clear();
    destructibles_.clear();
    call_temps_.clear();
    pattern_bases_.clear();
    pattern_results_.clear();
    break_targets_.clear();
    continue_targets_.clear();

    MIRBuilder b(*saved_arena, module_, fn);
    builder_ = &b;
    current_func_ = lamb;
    if (set_current_func_) set_current_func_(lamb);
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    sret_ = l_sret;
    sret_ret_type_ = ret;
    return_type_ = lamb->returnType;
    if (l_sret) {
        sret_ptr_type_ = types_.pointer_type(ret, true);
        sret_ptr_ = b.param(sret_ptr_type_);
    }

    bool ok = true;
    for (FunctionParam* p : lamb->params) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        if (place == MIR_INVALID_ID) {
            ok = false;
            error = "failed to allocate lambda parameter place";
            break;
        }
        b.store(place, pv);
        bind(p, place);
        bind_name(p->name, place);
        if (p->type) {
            const TypeId ptt = types_.map(p->type);
            const MIRTypeRecord& prr = module_.types.get(ptt);
            if (!(prr.flags & TF_TYPEDEF) && needs_aggregate_path(module_, ptt)) {
                register_destructible(place, p->type);
            }
        }
    }

    if (ok) ok = lower_scope(lamb->scope, error);
    if (ok && !b.current_block_terminated()) {
        emit_drops();
        b.ret_void();
    }
    if (ok && !b.ok()) {
        ok = false;
        error = b.error() ? b.error() : "MIR builder failure";
    }

    std::string c;
    if (ok) {
        std::string e;
        if (!emit_function_c(fn, module_, c, e)) {
            ok = false;
            error = e;
        }
    }
    if (ok) lambda_defs_.push_back(std::move(c));

    // restore the enclosing function's state
    builder_ = saved_builder;
    arena_ = saved_arena;
    current_func_ = saved_cur;
    if (set_current_func_) set_current_func_(saved_cur);
    var_places_ = std::move(saved_var);
    name_places_ = std::move(saved_name);
    destructibles_ = std::move(saved_destructibles);
    call_temps_ = std::move(saved_call_temps);
    sret_ = saved_sret;
    sret_ptr_ = saved_sret_ptr;
    sret_ret_type_ = saved_sret_ret;
    sret_ptr_type_ = saved_sret_ptr_t;
    return_type_ = saved_return_type;
    break_targets_ = std::move(saved_break);
    continue_targets_ = std::move(saved_continue);

    if (!ok) return MIRExprResult::error();

    TypeId lam_type = types_.map(lamb->getType());
    if (lam_type == MIR_INVALID_ID ||
        (module_.types.get(lam_type).kind != MIRTypeKind::Function &&
         module_.types.get(lam_type).kind != MIRTypeKind::Pointer)) {
        lam_type = types_.pointer_type(ftype, true);
    }
    ValueId v = builder_->function_addr(sym, lam_type);
    return MIRExprResult::value(v, lam_type);
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
    destructibles_.clear();
    // func_symbols_ persists across functions in a module so callees keep stable ids

    func.symbol = intern_function(decl);
    const TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    sret_ = needs_sret_return(module_, ret);
    sret_ret_type_ = ret;
    return_type_ = decl->returnType;
    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
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
            TypeId pt = self_param->type ? types_.map(self_param->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
            ptypes.push_back(pt);
        }
        for (FunctionParam* p : decl->params) {
            TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
            if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
            ptypes.push_back(pt);
        }
        const TypeId ftype = types_.function_signature(sret_ ? types_.void_type() : ret, ptypes);
        module_.symbols.symbols[func.symbol].type = ftype;
        func.function_type = ftype;
    }

    MIRBuilder b(arena, module_, func);
    builder_ = &b;

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
        } else if (needs_aggregate_path(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place = b.alloca(pt, MIRStorageClass::Parameter);
        b.store(place, pv);
        bind(self_param, place);
        bind_name(self_param->name, place);
    }

    // parameters: materialize an SSA param value, spill to a place, bind it
    for (FunctionParam* p : decl->params) {
        const TypeId declared = p && p->type ? types_.map(p->type) : types_.opaque_type();
        const bool is_array = declared != MIR_INVALID_ID &&
                              module_.types.get(declared).kind == MIRTypeKind::Array;
        const bool is_aggregate = declared != MIR_INVALID_ID && !is_array &&
                                  !(module_.types.get(declared).flags & TF_TYPEDEF) &&
                                  needs_aggregate_path(module_, declared);
        TypeId pt = declared;
        if (is_array) {
            pt = types_.pointer_type(module_.types.get(declared).element, true);
        } else if (is_aggregate) {
            pt = types_.pointer_type(declared, true);
        }
        ValueId pv = b.param(pt);
        PlaceId place;
        if (is_aggregate) {
            // the aggregate is passed by a hidden pointer; materialize a
            // by-value copy so the body sees the value, not the pointer
            place = b.alloca(declared, MIRStorageClass::Parameter);
            ValueId val = b.load_indirect(pv, declared);
            b.store(place, val);
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
            if (!(prr.flags & TF_TYPEDEF) && needs_aggregate_path(module_, ptt)) {
                register_destructible(place, p->type);
            }
        }
    }

    if (decl->body.has_value()) {
        for (ASTNode* node : decl->body->nodes) {
            if (!lower_stmt(node, error)) {
                builder_ = nullptr;
                return false;
            }
        }
    }

    if (!b.current_block_terminated()) {
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
