// Copyright (c) Chemical Language Foundation 2025.
//
// AST-aware helpers shared by the split MIRLowerer translation units
// (action D4). These lived in an anonymous namespace inside MIRLowerer.cpp;
// they are `inline` so every per-construct TU sees exactly one definition.
// A helper moves here only when a second lowering TU needs it.

#pragma once

#include "MIRLowerer.h"
#include "MIRLowererInternal.h"
#include "MIRABI.h"
#include "MIRPrototype.h"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <chrono>

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
inline bool struct_derives_from(StructDefinition* derived, BaseType* base) {
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

inline BaseType* resolve_sizeof_operand(BaseType* t) {
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
inline Value* captured_comptime_value(ASTNode* linked) {
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
inline bool inherited_field_path(ASTNode* container, ASTNode* member, std::string& out) {
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
inline bool is_implicit_field_node(ASTNode* n) {
    if (!n) return false;
    return n->kind() == ASTNodeKind::StructMember ||
           n->kind() == ASTNodeKind::UnnamedStruct ||
           n->kind() == ASTNodeKind::UnnamedUnion;
}

inline bool map_binary(Operation op, MIROpcode& out, MIRBinaryOp& bop) {    switch (op) {
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
inline const char* operator_method_name(Operation op) {
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
inline const char* compound_method_name(Operation op) {
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

inline bool compound_binary(Operation op, MIRBinaryOp& out) {
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

inline AtomicIntrinsic atomic_intrinsic_of(FunctionDeclaration* fd) {
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

} // namespace mir
