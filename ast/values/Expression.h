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

    /**
     * this replaces the first or second value, by basically compile time casting
     * it asks the type of the other value which is a IntN type to create the value
     * with the constant number value of other value
     *
     * when expression involves a NumberValue, we replace it with the other value's type
     * for example 123 (int32) == 123 (number value) becomes 123 (int32) == 123 (int32)
     * number value by defaults results in int32 but number value also stores values as int64
     *
     * this means data can be lost, for example comparing int32 with a constant bigint
     * 123 (int32) == 999999999999, the second number won't fit in int32, we will accept data loss
     * TODO we will perform a check if the constant cannot fit in the type, we'll raise an error
     *
     * the values are replaced before code_gen
     *
     * @param firstType type of the first value
     * @param secondType type of the second value
     */
    void replace_number_values(BaseType* firstType, BaseType* secondType);

    /**
     *
     * when a literal value is being compared with a variable
     * but variable has less bits (e.g short variable with constant int32)
     * we will demote int 32 to a short type, but only if it fits in the range of a short
     * TODO only do this if the constant value is in range of the type
     *
     * the values are replaced before code_gen
     *
     * @param firstType type of the first value
     * @param secondType type of the second value
     */
    void shrink_literal_values(BaseType* firstType, BaseType* secondType);

    /**
     * promote values when the types don't match
     * for example a float is being compared with an int32
     * 2.0 == 2 or x (float) == 2.0
     *
     * this promotion is done before code_gen
     *
     */
    void promote_literal_values(BaseType* firstType, BaseType* secondType);

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