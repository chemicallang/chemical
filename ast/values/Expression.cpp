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
#include "ast/values/FloatValue.h"
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
        if(firstValue->as_number_val() != nullptr) {
            auto value = ((IntNumValue*)firstValue)->get_num_value();
            firstValue = ((IntNType*) secondType)->create(allocator, value);
        } else if(secondValue->as_number_val() != nullptr){
            auto value = ((IntNumValue*)secondValue)->get_num_value();
            secondValue = ((IntNType*) firstType)->create(allocator, value);
        }
    }
    const auto first = getEnumDecl(firstType);
    if(first) {
        const auto second = secondValue->as_number_val();
        if(second) {
            secondValue = first->underlying_type->create(allocator, second->value);
        }
    } else {
        const auto second = getEnumDecl(secondType);
        if(second) {
            const auto firstVal = firstValue->as_number_val();
            if(firstVal) {
                firstValue = second->underlying_type->create(allocator, firstVal->value);
            }
        }
    }
}

void Expression::shrink_literal_values(ASTAllocator& allocator, BaseType* firstType, BaseType* secondType) {
    if(!(!firstValue->primitive() && !secondValue->primitive())) { // if at least one of the value is a literal
        if (firstValue->is_int_n() && secondValue->is_int_n()) { // if both are int n
            if(firstValue->primitive()) {
                auto secIntNTy = (IntNType*) secondType;
                auto firstVal = (IntNumValue*) firstValue;
                if(firstVal->get_num_bits() > secIntNTy->num_bits() || (firstVal->get_num_bits() == secIntNTy->num_bits() && !firstVal->is_unsigned() && secIntNTy->is_unsigned())) {
                    firstValue = secIntNTy->create(allocator, firstVal->get_num_value());
                }
            } else {
                auto firIntTy = (IntNType*) firstType;
                auto secondVal = (IntNumValue*) secondValue;
                if(secondVal->get_num_bits() > firIntTy->num_bits() || (secondVal->get_num_bits() == firIntTy->num_bits() && !secondVal->is_unsigned() && firIntTy->is_unsigned())) {
                    secondValue = firIntTy->create(allocator, secondVal->get_num_value());
                }
            }
        }
    }
}

void Expression::promote_literal_values(ASTAllocator& allocator, BaseType* firstType, BaseType* secondType) {
#ifdef DEBUG
    if(firstType->can_promote(secondValue) && secondType->can_promote(firstValue)) {
        throw std::runtime_error("Both values can promote each other");
    }
#endif
    if (firstType->can_promote(secondValue)) {
        secondValue = firstType->promote(allocator, secondValue);
    } else if(secondType->can_promote(firstValue)) {
        firstValue = secondType->promote(allocator, firstValue);
    }
}

BaseType* Expression::create_type(ASTAllocator& allocator) {
    if(operation >= Operation::IndexComparisonStart && operation <= Operation::IndexComparisonEnd) {
        return new (allocator.allocate<BoolType>()) BoolType(location);
    }
    auto first = firstValue->create_type(allocator);
    auto second = secondValue->create_type(allocator);
    if(first == nullptr || second == nullptr) {
        return nullptr;
    }
    if((operation == Operation::Addition || operation == Operation::Subtraction) && first->kind() == BaseTypeKind::Pointer) {
        auto second_value_type = second->value_type();
        if(second_value_type >= ValueType::IntNStart && second_value_type <= ValueType::IntNEnd) {
            return first;
        }
    }
    if(first->value_type() == ValueType::Pointer && second->value_type() == ValueType::Pointer) {
        return new (allocator.allocate<LongType>()) LongType(is64Bit, location);
    }
    if(first->can_promote(secondValue)) {
        auto promoted = first->promote(allocator, secondValue);
        return promoted->create_type(allocator);
    } else {
        if(second->can_promote(firstValue)) {
            auto promoted = second->promote(allocator, firstValue);
            return promoted->create_type(allocator);
        } else {
            return first;
        }
    }
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

/**
  * @brief Construct a new Expression object.
  *
  * @param firstValue The first value in the expression.
  * @param secondValue The second value in the expression.
  * @param operation The operation between the two values.
  */
Expression::Expression(
        Value* firstValue,
        Value* secondValue,
        Operation operation,
        bool is64Bit,
        SourceLocation location
) : firstValue(firstValue), secondValue(secondValue), operation(operation), is64Bit(is64Bit), location(location) {

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

bool Expression::computed() {
    return true;
}

int64_t operate(Operation op, int64_t first, int64_t second) {
    switch(op) {
        case Operation::Addition:
            return first + second;
        case Operation::Subtraction:
            return first - second;
        case Operation::Multiplication:
            return first * second;
        case Operation::Division:
            return first / second;
        case Operation::IsEqual:
            return first == second;
        case Operation::IsNotEqual:
            return first != second;
        case Operation::GreaterThan:
            return first > second;
        case Operation::LessThan:
            return first < second;
        case Operation::GreaterThanOrEqual:
            return first >= second;
        case Operation::LessThanOrEqual:
            return first <= second;
        case Operation::Modulus:
            return first % second;
        case Operation::LeftShift:
            return first << second;
        case Operation::RightShift:
            return first >> second;
        case Operation::BitwiseAND:
            return first & second;
        case Operation::BitwiseOR:
            return first | second;
        default:
#ifdef DEBUG
        throw std::runtime_error("UNKNOWN INTERPRET OPERATION");
#endif
            return 0;
    }
}

double operate(Operation op, double first, double second) {
    switch(op) {
        case Operation::Addition:
            return first + second;
        case Operation::Subtraction:
            return first - second;
        case Operation::Multiplication:
            return first * second;
        case Operation::Division:
            return first / second;
        case Operation::IsEqual:
            return first == second;
        case Operation::IsNotEqual:
            return first != second;
        case Operation::GreaterThan:
            return first > second;
        case Operation::LessThan:
            return first < second;
        case Operation::GreaterThanOrEqual:
            return first >= second;
        case Operation::LessThanOrEqual:
            return first <= second;
        default:
#ifdef DEBUG
            throw std::runtime_error("UNKNOWN INTERPRET OPERATION");
#endif
            return 0;
    }
}

ValueKind determine_output(Operation op, ValueKind first, ValueKind second) {
    switch(op) {
        case Operation::IsEqual:
        case Operation::IsNotEqual:
        case Operation::GreaterThan:
        case Operation::GreaterThanOrEqual:
        case Operation::LessThan:
        case Operation::LessThanOrEqual:
            return ValueKind::Bool;
        default:
            return first > second ? first : second;
    }
}

Value* evaluate(InterpretScope& scope, Operation operation, Value* fEvl, Value* sEvl) {
    const auto fKind = fEvl->val_kind();
    const auto sKind = sEvl->val_kind();
    if(fKind >= ValueKind::IntNStart && fKind <= ValueKind::IntNEnd && sKind >= ValueKind::IntNStart && sKind <= ValueKind::IntNEnd) {
        // both values are int num values
        const auto first = (IntNumValue*) fEvl;
        const auto second = (IntNumValue*) sEvl;
        const auto answer = operate(operation, first->get_num_value(), second->get_num_value());
        return pack_by_kind(scope, determine_output(operation, fKind, sKind), answer);
    } else if(fKind == ValueKind::Double || fKind == ValueKind::Float || sKind == ValueKind::Double || sKind == ValueKind::Float) {
        const auto first = get_double_value(fEvl, fKind);
        const auto second = get_double_value(sEvl, sKind);
        const auto answer = operate(operation, first, second);
        return pack_by_kind(scope, determine_output(operation, fKind, sKind), answer);
    } else {
#ifdef DEBUG
        throw std::runtime_error("OPERATION BETWEEN VALUES OF UNKNOWN KIND");
#endif
        return nullptr;
    }
}

/**
 * evaluates both values and returns the result as unique_tr to Value
 * @return
 */
Value *Expression::evaluate(InterpretScope &scope) {
    auto fEvl = firstValue->evaluated_value(scope);
    auto sEvl = secondValue->evaluated_value(scope);
    return ::evaluate(scope, operation, fEvl, sEvl);
}

Expression *Expression::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<Expression>()) Expression(
        firstValue->copy(allocator),
        secondValue->copy(allocator),
        operation,
        is64Bit,
        location
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