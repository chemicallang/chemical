// Copyright (c) Qinetik 2024.

#include "Expression.h"
#include "IntNumValue.h"
#include "ast/types/IntNType.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/base/ASTNode.h"
#include "ast/types/BoolType.h"
#include "ast/types/LongType.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/StringValue.h"
#include "ast/base/InterpretScope.h"
#include "compiler/SymbolResolver.h"

EnumDeclaration* getEnumDecl(BaseType* type) {
    const auto linked = type->linked_node();
    if(linked && linked->kind() == ASTNodeKind::EnumDecl) {
        return linked->as_enum_decl_unsafe();
    } else {
        return nullptr;
    }
}

void Expression::replace_number_values(ASTAllocator& allocator, BaseType* firstType, BaseType* secondType) {
    if(firstType->kind() == BaseTypeKind::IntN && secondType->kind() == BaseTypeKind::IntN) {
        if(Value::isNumberValue(firstValue->val_kind())) {
            auto value = ((IntNumValue*)firstValue)->get_num_value();
            firstValue = ((IntNType*) secondType)->create(allocator, value);
        } else if(Value::isNumberValue(secondValue->val_kind())){
            auto value = ((IntNumValue*)secondValue)->get_num_value();
            secondValue = ((IntNType*) firstType)->create(allocator, value);
        }
    }
    const auto first = getEnumDecl(firstType);
    if(first) {
        const auto second = secondValue->as_number_value();
        if(second) {
            secondValue = first->underlying_type->create(allocator, second->value);
        }
    } else {
        const auto second = getEnumDecl(secondType);
        if(second) {
            const auto firstVal = firstValue->as_number_value();
            if(firstVal) {
                firstValue = second->underlying_type->create(allocator, firstVal->value);
            }
        }
    }
}

BaseType* Expression::create_type(ASTAllocator& allocator) {
    if(operation >= Operation::IndexBooleanReturningStart && operation <= Operation::IndexBooleanReturningEnd) {
        return new (allocator.allocate<BoolType>()) BoolType(location);
    }
    auto firstType = firstValue->create_type(allocator);
    auto secondType = secondValue->create_type(allocator);
    if(firstType == nullptr || secondType == nullptr) {
        return nullptr;
    }
    const auto first = firstType->pure_type();
    const auto second = secondType->pure_type();
    const auto first_kind = first->kind();
    const auto second_kind = second->kind();
    // operation between integer and float/double results in float/double
    if(first_kind == BaseTypeKind::IntN && (second_kind == BaseTypeKind::Float || second_kind == BaseTypeKind::Double)) {
        return second;
    } else if(second_kind == BaseTypeKind::IntN && (first_kind == BaseTypeKind::Float || first_kind == BaseTypeKind::Double)) {
        return first;
    }
    // operation between two integers of different int n types results in int n type of higher bit width
    if(first_kind == BaseTypeKind::IntN && second_kind == BaseTypeKind::IntN) {
        const auto first_intN = first->as_intn_type_unsafe();
        const auto second_intN = second->as_intn_type_unsafe();
        return first_intN->num_bits() > second_intN->num_bits() ? first : second;
    }
    // addition or subtraction of integer value into a pointer
    if((operation == Operation::Addition || operation == Operation::Subtraction) && (first_kind == BaseTypeKind::Pointer && second_kind == BaseTypeKind::IntN) || (first_kind == BaseTypeKind::IntN && second_kind == BaseTypeKind::Pointer)) {
        return first;
    }
    // subtracting a pointer results in a long type
    if(operation == Operation::Subtraction && first_kind == BaseTypeKind::Pointer && second_kind == BaseTypeKind::Pointer) {
        return new (allocator.allocate<LongType>()) LongType(is64Bit, location);
    }
    return first;
}

BaseType* Expression::known_type() {
    return created_type;
}

uint64_t Expression::byte_size(bool is64Bit) {
    return created_type->byte_size(is64Bit);
}

ASTNode* Expression::linked_node() {
    return created_type->linked_node();
}

bool Expression::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    is64Bit = linker.is64Bit;
    auto f = firstValue->link(linker, firstValue);
    auto s = secondValue->link(linker, secondValue);
    auto result = f && s;
    // ast allocator is being used
    // it's unknown when this expression should be disposed
    // file level / module level allocator should be used, when this expression belongs to a function
    // or decl that is private or internal, however that is hard to determine
    created_type = create_type(*linker.ast_allocator);
    return result;
}

bool Expression::primitive() {
    return false;
}

/**
 * evaluates both values and returns the result as unique_tr to Value
 * @return
 */
Value *Expression::evaluate(InterpretScope &scope) {
    auto fEvl = firstValue->evaluated_value(scope);
    auto sEvl = secondValue->evaluated_value(scope);
    return scope.evaluate(operation, fEvl, sEvl, location);
}

Expression *Expression::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<Expression>()) Expression(
        firstValue->copy(allocator),
        secondValue->copy(allocator),
        operation,
        is64Bit,
        location,
        created_type
    );
}

bool Expression::compile_time_computable() {
    return firstValue->compile_time_computable() && secondValue->compile_time_computable();
}

/**
 * evaluates the current expression and also interprets the evaluated value
 * @param scope
 */
void Expression::interpret(InterpretScope &scope) {
    evaluate(scope)->interpret(scope);
}