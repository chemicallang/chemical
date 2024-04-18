// Copyright (c) Qinetik 2024.

#include "Expression.h"
#include "ast/base/GlobalInterpretScope.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *Expression::llvm_value(Codegen &gen) {
    promote();
    return gen.operate(operation, firstValue.get(), secondValue.get());
}

llvm::Type *Expression::llvm_type(Codegen &gen) {
    return create_type()->llvm_type(gen);
}

#endif

void Expression::promote() {
    auto first = firstValue->create_type();
    auto second = secondValue->create_type();
    if (first->is_same(second.get())) {
        // no promotion required
    } else {
        if (first->precedence() >= second->precedence()) {
            secondValue = first->promote(secondValue.release());
        } else {
            firstValue = second->promote(firstValue.release());
        }
    }
}

std::unique_ptr<BaseType> Expression::create_type() const {
    auto first = firstValue->create_type();
    auto second = secondValue->create_type();
    if (first->is_same(second.get())) {
        return first;
    } else {
        if (first->precedence() >= second->precedence()) {
            return first;
        } else {
            return second;
        }
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