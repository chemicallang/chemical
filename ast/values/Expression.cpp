// Copyright (c) Qinetik 2024.

#include "Expression.h"
#include "ast/base/GlobalInterpretScope.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "IntNumValue.h"
#include "ast/types/IntNType.h"
#include "ast/values/UShortValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/IntValue.h"
#include "ast/values/ULongValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/BigIntValue.h"

llvm::Value *Expression::llvm_value(Codegen &gen) {
    promote(gen.is64Bit);
    return gen.operate(operation, firstValue.get(), secondValue.get());
}

llvm::Type *Expression::llvm_type(Codegen &gen) {
    return create_type()->llvm_type(gen);
}

#endif

Value* create_with_bits(uint64_t value, unsigned int num_bits, bool is_unsigned, bool is64bit) {
    switch(num_bits) {
        case 16:
            if(is_unsigned) {
                return new UShortValue(value);
            } else {
                return new ShortValue(value);
            }
        case 32:
            if(is_unsigned) {
                return new UIntValue(value);
            } else {
                return new IntValue(value);
            }
        case 64:
            if(is64bit) {
                if(is_unsigned) {
                    return new ULongValue(value, is64bit);
                } else {
                    return new LongValue(value, is64bit);
                }
            } else {
                if(is_unsigned) {
                    return new UBigIntValue(value);
                } else {
                    return new BigIntValue(value);
                }
            }
        default:
            throw std::runtime_error("unhandled bit number " + std::to_string(value) + " when creating intN value");

    }
}

void Expression::promote(bool is64Bit) {
#ifdef DEBUG
    if(firstValue->can_promote(secondValue.get()) && secondValue->can_promote(firstValue.get())) {
        throw std::runtime_error("Both values can promote each other");
    }
#endif
    // TODO only do this if the constant value is in range of the type
    // when a literal value is being compared with a variable
    // but variable has less bits (e.g short variable with constant int32)
    // we will demote int 32 to a short type, but only if it first in the range of a short
    // if one of the values is a literal value
    if(!(!firstValue->primitive() && !secondValue->primitive())) { // if at least one of the value is a literal
        if (firstValue->is_int_n() && secondValue->is_int_n()) { // if both are int n
            if(firstValue->primitive()) {
                auto secondType = secondValue->create_type();
                auto secIntNTy = (IntNType*) secondType.get();
                auto firstVal = (IntNumValue*) firstValue.get();
                if(firstVal->get_num_bits() > secIntNTy->number || (firstVal->get_num_bits() == secIntNTy->number && !firstVal->is_unsigned() && secIntNTy->is_unsigned)) {
                    firstValue = std::unique_ptr<Value>(create_with_bits(firstVal->get_num_value(), secIntNTy->number, secIntNTy->is_unsigned, is64Bit));
                }
            } else {
                auto firstType = firstValue->create_type();
                auto firIntTy = (IntNType*) firstType.get();
                auto secondVal = (IntNumValue*) secondValue.get();
                if(secondVal->get_num_bits() > firIntTy->number || (secondVal->get_num_bits() == firIntTy->number && !secondVal->is_unsigned() && firIntTy->is_unsigned)) {
                    secondValue = std::unique_ptr<Value>(create_with_bits(secondVal->get_num_value(), firIntTy->number, firIntTy->is_unsigned, is64Bit));
                }
            }
        }
    }
    if (firstValue->can_promote(secondValue.get())) {
        secondValue = std::unique_ptr<Value>(firstValue->promote(secondValue.get()));
    } else if(secondValue->can_promote(firstValue.get())) {
        firstValue = std::unique_ptr<Value>(secondValue->promote(firstValue.get()));
    }
}

std::unique_ptr<BaseType> Expression::create_type() const {
    if (firstValue->can_promote(secondValue.get())) {
        return std::unique_ptr<Value>(firstValue->promote(secondValue.get()))->create_type();
    } else if(secondValue->can_promote(firstValue.get())) {
        return std::unique_ptr<Value>(secondValue->promote(firstValue.get()))->create_type();
    } else {
        return firstValue->create_type();
    }
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
        Operation operation
) : firstValue(std::move(firstValue)), secondValue(std::move(secondValue)), operation(operation) {

}

void Expression::link(SymbolResolver &linker) {
    firstValue->link(linker);
    secondValue->link(linker);
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
inline Value *Expression::evaluate(InterpretScope &scope) {
    auto fEvl = firstValue->evaluated_value(scope);
    auto sEvl = secondValue->evaluated_value(scope);
    auto index = ExpressionEvaluator::index(fEvl->value_type(), sEvl->value_type(), operation);
    auto found = scope.global->expr_evaluators.find(index);
    if (found != scope.global->expr_evaluators.end()) {
        auto result = scope.global->expr_evaluators[index](fEvl, sEvl);
        if (firstValue->computed()) delete fEvl;
        if (secondValue->computed()) delete sEvl;
        return result;
    } else {
        scope.error(
                "Cannot evaluate expression as the method with index " + std::to_string(index) +
                " does not exist, for value types " + to_string(fEvl->value_type()) + " and " +
                to_string(sEvl->value_type()));
        return nullptr;
    }
}

Value *Expression::evaluated_value(InterpretScope &scope) {
    return evaluate(scope);
}

bool Expression::evaluated_bool(InterpretScope &scope) {
    // compute the expression value
    auto eval = evaluate(scope);
    // get and store the expression value as primitive boolean
    auto value = eval->as_bool();
    // delete the expression value
    delete eval;
    // return the expression value
    return value;
}

Value *Expression::initializer_value(InterpretScope &scope) {
    return evaluated_value(scope);
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

std::string Expression::representation() const {
    std::string rep;
    rep.append(1, '(');
    rep.append(firstValue->representation());
    rep.append(to_string(operation));
    rep.append(secondValue->representation());
    rep.append(1, ')');
    return rep;
}