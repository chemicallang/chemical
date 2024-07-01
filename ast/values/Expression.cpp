// Copyright (c) Qinetik 2024.

#include "Expression.h"
#include "ast/base/GlobalInterpretScope.h"
#include "IntNumValue.h"
#include "ast/types/IntNType.h"
#include "ast/types/BoolType.h"
#include "ast/types/LongType.h"

void Expression::replace_number_values(BaseType* firstType, BaseType* secondType) {
    if(firstType->kind() == BaseTypeKind::IntN && secondType->kind() == BaseTypeKind::IntN) {
        if(firstValue->as_number_val() != nullptr) {
            auto value = ((IntNumValue*)firstValue.get())->get_num_value();
            firstValue = std::unique_ptr<Value>(((IntNType*) secondType)->create(value));
        } else if(secondValue->as_number_val() != nullptr){
            auto value = ((IntNumValue*)secondValue.get())->get_num_value();
            secondValue = std::unique_ptr<Value>(((IntNType*) firstType)->create(value));
        }
    }
}

void Expression::shrink_literal_values(BaseType* firstType, BaseType* secondType) {
    if(!(!firstValue->primitive() && !secondValue->primitive())) { // if at least one of the value is a literal
        if (firstValue->is_int_n() && secondValue->is_int_n()) { // if both are int n
            if(firstValue->primitive()) {
                auto secIntNTy = (IntNType*) secondType;
                auto firstVal = (IntNumValue*) firstValue.get();
                if(firstVal->get_num_bits() > secIntNTy->num_bits() || (firstVal->get_num_bits() == secIntNTy->num_bits() && !firstVal->is_unsigned() && secIntNTy->is_unsigned())) {
                    firstValue = std::unique_ptr<Value>(secIntNTy->create(firstVal->get_num_value()));
                }
            } else {
                auto firIntTy = (IntNType*) firstType;
                auto secondVal = (IntNumValue*) secondValue.get();
                if(secondVal->get_num_bits() > firIntTy->num_bits() || (secondVal->get_num_bits() == firIntTy->num_bits() && !secondVal->is_unsigned() && firIntTy->is_unsigned())) {
                    secondValue = std::unique_ptr<Value>(firIntTy->create(secondVal->get_num_value()));
                }
            }
        }
    }
}

void Expression::promote_literal_values(BaseType* firstType, BaseType* secondType) {
#ifdef DEBUG
    if(firstType->can_promote(secondValue.get()) && secondType->can_promote(firstValue.get())) {
        throw std::runtime_error("Both values can promote each other");
    }
#endif
    if (firstType->can_promote(secondValue.get())) {
        secondValue = std::unique_ptr<Value>(firstType->promote(secondValue.get()));
    } else if(secondType->can_promote(firstValue.get())) {
        firstValue = std::unique_ptr<Value>(secondType->promote(firstValue.get()));
    }
}

std::unique_ptr<BaseType> Expression::create_type() {
    if(operation >= Operation::IndexComparisonStart && operation <= Operation::IndexComparisonEnd) {
        return std::make_unique<BoolType>();
    }
    auto first = firstValue->create_type();
    auto second = secondValue->create_type();
    if(first->value_type() == ValueType::Pointer && second->value_type() == ValueType::Pointer) {
        return std::make_unique<LongType>(is64Bit);
    }
    if(first->can_promote(secondValue.get())) {
        return std::unique_ptr<Value>(first->promote(secondValue.get()))->create_type();
    } else {
        if(second->can_promote(firstValue.get())) {
            return std::unique_ptr<Value>(second->promote(firstValue.get()))->create_type();
        } else {
            return first;
        }
    }
}

hybrid_ptr<BaseType> Expression::get_base_type() {
    return hybrid_ptr<BaseType> { create_type().release(), true };
}

uint64_t Expression::byte_size(bool is64Bit) {
    return create_type()->byte_size(is64Bit);
}

/**
  * @brief Construct a new Expression object.
  *
  * @param firstValue The first value in the expression.
  * @param secondValue The second value in the expression.
  * @param operation The operation between the two values.
  */
Expression::Expression(
        std::unique_ptr<Value> firstValue,
        std::unique_ptr<Value> secondValue,
        Operation operation,
        bool is64Bit
) : firstValue(std::move(firstValue)), secondValue(std::move(secondValue)), operation(operation), is64Bit(is64Bit) {

}

void Expression::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    firstValue->link(linker, firstValue);
    secondValue->link(linker, secondValue);
}

bool Expression::primitive() {
    return false;
}

bool Expression::computed() {
    return true;
}

/**
 * evaluates both values and returns the result as unique_tr to Value
 * @return
 */
Value *Expression::evaluate(InterpretScope &scope) {
    auto fEvl = firstValue->evaluated_value(scope);
    auto sEvl = secondValue->evaluated_value(scope);
    auto index = ExpressionEvaluator::index(fEvl->value_type(), sEvl->value_type(), operation);
    auto found = scope.global->expr_evaluators.find(index);
    if (found != scope.global->expr_evaluators.end()) {
        return scope.global->expr_evaluators[index](fEvl.get(), sEvl.get());
    } else {
        scope.error(
                "Cannot evaluate expression as the method with index " + std::to_string(index) +
                " does not exist, for value types " + to_string(fEvl->value_type()) + " and " +
                to_string(sEvl->value_type()));
        return nullptr;
    }
}

Value *Expression::copy() {
    return new Expression(
        std::unique_ptr<Value>(firstValue->copy()),
        std::unique_ptr<Value>(secondValue->copy()),
        operation,
        is64Bit
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