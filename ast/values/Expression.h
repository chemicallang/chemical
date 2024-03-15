// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
#include "ast/utils/ExpressionEvaluator.h"
#include <memory>

class Expression : public Value {
public:

    /**
     * @brief Construct a new Expression object.
     *
     * @param firstValue The first value in the expression.
     * @param secondValue The second value in the expression.
     * @param operation The operation between the two values.
     */
    Expression(
            std::unique_ptr<Value> firstValue,
            std::unique_ptr<Value> secondValue,
            Operation operation
    ) : firstValue(std::move(firstValue)), secondValue(std::move(secondValue)), operation(operation) {

    }

#ifdef COMPILER_BUILD
    llvm::Value * llvm_value(Codegen &gen) override {
        return gen.operate(operation, firstValue->llvm_value(gen), secondValue->llvm_value(gen));
    }
#endif

    /**
     * evaluates both values and returns the result as unique_tr to Value
     * @return
     */
    inline Value* evaluate(InterpretScope& scope) {
        auto fEvl = firstValue->evaluated_value(scope);
        auto sEvl = secondValue->evaluated_value(scope);
        auto index = ((uint8_t) fEvl->value_type() << 20) | ((uint8_t) sEvl->value_type() << 10) | (uint8_t) operation;
        if (ExpressionEvaluator::functionVector.contains(index)) {
            return ExpressionEvaluator::functionVector[index](fEvl, sEvl);
        } else {
            throw std::runtime_error(
                    "Cannot evaluate expression as the method with index " + std::to_string(index) + " does not exist");
        }
    }

    Value *evaluated_value(InterpretScope& scope) override {
        return evaluate(scope);
    }

    bool evaluated_bool(InterpretScope& scope) override {
        // compute the expression value
        auto eval = evaluate(scope);
        // get and store the expression value as primitive boolean
        auto value = eval->as_bool();
        // delete the expression value
        delete eval;
        // return the expression value
        return value;
    }

    /**
     * evaluates the current expression and also interprets the evaluated value
     * @param scope
     */
    void interpret(InterpretScope &scope) override {
        evaluate(scope)->interpret(scope);
    }

    std::string representation() const override {
        std::string rep;
        rep.append(1, '(');
        rep.append(firstValue->representation());
        rep.append(to_string(operation));
        rep.append(secondValue->representation());
        rep.append(1, ')');
        return rep;
    }


private:
    std::unique_ptr<Value> firstValue; ///< The first value in the expression.
    std::unique_ptr<Value> secondValue; ///< The second value in the expression.
    Operation operation; ///< The operation between the two values.
};