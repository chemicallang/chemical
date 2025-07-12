// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
#include "std/chem_string_view.h"
#include <memory>

class Expression : public Value {
public:

    Value* firstValue; ///< The first value in the expression.
    Value* secondValue; ///< The second value in the expression.
    Operation operation; ///< The operation between the two values.
    BaseType* created_type = nullptr;

    /**
     * constructor
     */
    constexpr Expression(
            Value* firstValue,
            Value* secondValue,
            Operation operation,
            SourceLocation location,
            BaseType* created_type = nullptr
    ) : Value(ValueKind::Expression, location), firstValue(firstValue), secondValue(secondValue),
        operation(operation), created_type(created_type) {

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
    void replace_number_values(ASTAllocator& allocator, BaseType* firstType, BaseType* secondType);

    uint64_t byte_size(bool is64Bit) final;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_logical_expr(Codegen &gen, BaseType* firstType, BaseType* secondType);

    void llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block);

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    bool primitive() final;

    ASTNode* linked_node() final;

    /**
     * evaluates both values and returns the result as unique_tr to Value
     */
    Value *evaluate(InterpretScope &scope);

    Expression *copy(ASTAllocator& allocator) final;

    Value* evaluated_value(InterpretScope &scope) final {
        return evaluate(scope);
    }

    bool compile_time_computable() final;

};