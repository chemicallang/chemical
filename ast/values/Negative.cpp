// Copyright (c) Chemical Language Foundation 2025.

#include "Negative.h"
#include "ast/base/BaseType.h"
#include "IntNumValue.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/MembersContainer.h"
#include "ast/types/IntNType.h"
#include "compiler/ASTDiagnoser.h"

uint64_t NegativeValue::byte_size(bool is64Bit) {
// TODO check this out
    return value->byte_size(is64Bit);
}

//TODO: make this function on IntNType
BaseType* to_signed(TypeBuilder& typeBuilder, IntNType* type) {
    switch(type->IntNKind()) {
        case IntNTypeKind::UChar:
            return (BaseType*) typeBuilder.getCharType();
        case IntNTypeKind::UShort:
            return (BaseType*) typeBuilder.getShortType();
        case IntNTypeKind::UInt:
            return (BaseType*) typeBuilder.getIntType();
        case IntNTypeKind::ULong:
            return (BaseType*) typeBuilder.getLongType();
        case IntNTypeKind::ULongLong:
            return (BaseType*) typeBuilder.getLongLongType();
        case IntNTypeKind::UInt128:
            return (BaseType*) typeBuilder.getInt128Type();
        case IntNTypeKind::U8:
            return (BaseType*) typeBuilder.getI8Type();
        case IntNTypeKind::U16:
            return (BaseType*) typeBuilder.getI16Type();
        case IntNTypeKind::U32:
            return (BaseType*) typeBuilder.getI32Type();
        case IntNTypeKind::U64:
            return (BaseType*) typeBuilder.getI64Type();
        default:
            return type;
    }
}

void NegativeValue::determine_type(TypeBuilder& typeBuilder, ASTDiagnoser& diagnoser) {
    const auto type = getValue()->getType();
    // check if operator is overloaded
    const auto node = type->get_linked_canonical_node(true, false);
    if(node) {
        const auto container = node->get_members_container();
        if(container) {
            const auto child = container->child("neg");
            if(!child || child->kind() != ASTNodeKind::FunctionDecl) {
                diagnoser.error(this) << "expected a function 'not' to be present for overloading";
                setType((BaseType*) typeBuilder.getVoidType());
                return;
            }
            const auto func = child->as_function_unsafe();
            if(func->params.size() != 1) {
                diagnoser.error(this) << "expected 'not' operator function to have a single parameter";
                setType((BaseType*) typeBuilder.getVoidType());
                return;
            }
            setType(func->returnType);
            return;
        }
    }
    // normal flow
    const auto can = type->canonical();
    setType(
            can->kind() == BaseTypeKind::IntN ? (
                    to_signed(typeBuilder, can->as_intn_type_unsafe())
            ) : type
    );
}

Value* NegativeValue::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    const auto eval_kind = eval->val_kind();
    if(eval_kind == ValueKind::IntN) {
        return pack_by_kind(scope, eval->as_int_num_value_unsafe()->getType()->IntNKind(), -((IntNumValue*) eval)->get_num_value(), encoded_location());
    } else if(eval_kind == ValueKind::Double || eval_kind == ValueKind::Float) {
        return pack_by_kind(scope, eval_kind, -get_double_value(eval, eval_kind), encoded_location());
    } else {
        return nullptr;
    }
}

bool NegativeValue::primitive() {
    return value->primitive();
}