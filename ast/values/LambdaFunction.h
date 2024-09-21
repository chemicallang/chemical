// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/Scope.h"
#include "ast/types/FunctionType.h"

class FunctionDeclaration;

class CapturedVariable;

/**
 * @brief Class representing an integer value.
 */
class LambdaFunction : public Value, public FunctionType {
public:

    std::vector<CapturedVariable*> captureList;
    Scope scope;

#ifdef COMPILER_BUILD

    llvm::Function* func_ptr = nullptr;
    llvm::Value *captured_struct = nullptr;

#endif

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    LambdaFunction(
            std::vector<CapturedVariable*> captureList,
            std::vector<FunctionParam*> params,
            bool isVariadic,
            Scope scope,
            CSTToken* token
    );

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::LambdaFunc;
    }

    ASTNode *parent() override {
        return nullptr;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    int data_struct_index() {
        return has_self_param() ? 1 : 0;
    }

#ifdef COMPILER_BUILD

    llvm::Type *capture_struct_type(Codegen &gen);

protected:

    /**
     * capture the variables in capture list into a single struct and return it
     */
    llvm::AllocaInst *capture_struct(Codegen &gen);

public:

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    BaseType* create_type(ASTAllocator &allocator) override;

    BaseType* known_type() override {
        return this;
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override;

    bool link(SymbolResolver &linker, FunctionType* func_type);

    [[nodiscard]]
    ValueType value_type() const override;

};