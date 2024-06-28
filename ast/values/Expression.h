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

    std::unique_ptr<Value> firstValue; ///< The first value in the expression.
    std::unique_ptr<Value> secondValue; ///< The second value in the expression.
    Operation operation; ///< The operation between the two values.
    bool is64Bit; // is 64bit operating system

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
            Operation operation,
            bool is64Bit
    );

    void accept(Visitor *visitor) override {
        visitor->visit(this);
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

    uint64_t byte_size(bool is64Bit) override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_logical_expr(Codegen &gen, BaseType* firstType, BaseType* secondType);

    void llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block);

#endif

    std::unique_ptr<BaseType> create_type() override;

    hybrid_ptr<BaseType> get_base_type() override;

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    bool primitive() override;

    bool computed() override;

    /**
     * evaluates both values and returns the result as unique_tr to Value
     */
    Value *evaluate(InterpretScope &scope);

    Value *scope_value(InterpretScope &scope) override {
        return evaluate(scope);
    }

    hybrid_ptr<Value> evaluated_value(InterpretScope &scope) override {
        return hybrid_ptr<Value> { evaluate(scope) };
    }

    bool compile_time_computable() override;

    ValueType value_type() const override {
        return ValueType::Expression;
    }

    /**
     * evaluates the current expression and also interprets the evaluated value
     * @param scope
     */
    void interpret(InterpretScope &scope) override;

};