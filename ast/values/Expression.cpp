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

bool operate(Operation op, bool first, bool second) {
    switch(op) {
        case Operation::LogicalAND:
            return first && second;
        case Operation::LogicalOR:
            return first || second;
        default:
#ifdef DEBUG
        throw std::runtime_error("unknown operation between bool values");
#endif
            return false;
    }
}

uint64_t operate(Operation op, uint64_t first, uint64_t second) {
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
        case Operation::BitwiseXOR:
            return first ^ second;
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

inline bool is_int_n(ValueKind k) {
    return k >= ValueKind::IntNStart && k <= ValueKind::IntNEnd;
}

inline BoolValue* pack_bool(InterpretScope& scope, bool value, SourceLocation location) {
    return new (scope.allocate<BoolValue>()) BoolValue(value, location);
}

Value* evaluate(InterpretScope& scope, Operation operation, Value* fEvl, Value* sEvl, SourceLocation location) {
    const auto fKind = fEvl->val_kind();
    const auto sKind = sEvl->val_kind();
    if(fKind == ValueKind::Bool && sKind == ValueKind::Bool) {
        const auto result = operate(operation, fEvl->get_the_bool(), sEvl->get_the_bool());
        return new (scope.allocate<BoolValue>()) BoolValue(result, location);
    } else if(is_int_n(fKind) && is_int_n(sKind)) {
        // both values are int num values
        const auto first = (IntNumValue*) fEvl;
        const auto second = (IntNumValue*) sEvl;
        const auto answer = operate(operation, first->get_num_value(), second->get_num_value());
        return pack_by_kind(scope, determine_output(operation, fKind, sKind), answer, location);
    } else if(fKind == ValueKind::Double || fKind == ValueKind::Float || sKind == ValueKind::Double || sKind == ValueKind::Float) {
        const auto first = get_double_value(fEvl, fKind);
        const auto second = get_double_value(sEvl, sKind);
        const auto answer = operate(operation, first, second);
        return pack_by_kind(scope, determine_output(operation, fKind, sKind), answer, location);
    } else if(fKind == ValueKind::NullValue || sKind == ValueKind::NullValue) {
        // comparison with null, a == null or null == a
        switch (operation) {
            case Operation::IsEqual:
                return pack_bool(scope, fKind == ValueKind::NullValue && sKind == ValueKind::NullValue, location);
            case Operation::IsNotEqual:
                return pack_bool(scope, fKind != sKind, location);
                break;
            default:
                return new (scope.allocate<NullValue>()) NullValue(location);
        }
    } else if((fKind == ValueKind::String && is_int_n(sKind))) {
        const auto strVal = fEvl->as_string_unsafe();
        const auto numVal = (IntNumValue*) sEvl;
        const auto num = numVal->get_num_value();
        return new (scope.allocate<StringValue>()) StringValue(chem::string_view(strVal->value.data() + num, strVal->value.size() - num), location);
    } else if ((sKind == ValueKind::String && is_int_n(fKind))) {
        const auto strVal = sEvl->as_string_unsafe();
        const auto numVal = (IntNumValue*) fEvl;
        const auto num = numVal->get_num_value();
        return new (scope.allocate<StringValue>()) StringValue(chem::string_view(strVal->value.data() + num, strVal->value.size() - num), location);
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
    return ::evaluate(scope, operation, fEvl, sEvl, location);
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