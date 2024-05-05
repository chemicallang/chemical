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
    );

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void promote(bool is64Bit);

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Type * llvm_type(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_type() const override;

    void link(SymbolResolver &linker) override;

    bool primitive() override;

    bool computed() override;

    /**
     * evaluates both values and returns the result as unique_tr to Value
     * @return
     */
    inline Value *evaluate(InterpretScope &scope);

    Value *evaluated_value(InterpretScope &scope) override;

    bool evaluated_bool(InterpretScope &scope) override;

    Value *initializer_value(InterpretScope &scope) override;

    bool compile_time_computable() override;

    /**
     * evaluates the current expression and also interprets the evaluated value
     * @param scope
     */
    void interpret(InterpretScope &scope) override;

    std::string representation() const override;


private:
    std::unique_ptr<Value> firstValue; ///< The first value in the expression.
    std::unique_ptr<Value> secondValue; ///< The second value in the expression.
    Operation operation; ///< The operation between the two values.
};