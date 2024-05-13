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
class LambdaFunction : public Value {
public:

    std::vector<std::unique_ptr<CapturedVariable>> captureList;
    std::vector<std::unique_ptr<FunctionParam>> params;
    bool isVariadic;
    Scope scope;

#ifdef COMPILER_BUILD

    llvm::Value *captured_struct = nullptr;

#endif

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    LambdaFunction(
            std::vector<std::unique_ptr<CapturedVariable>> captureList,
            std::vector<std::unique_ptr<FunctionParam>> params,
            bool isVariadic,
            Scope scope
    );

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    std::string representation() const override;

#ifdef COMPILER_BUILD

    llvm::Type *capture_struct_type(Codegen &gen);

protected:

    /**
     * capture the variables in capture list into a single struct and return it
     */
    llvm::AllocaInst *capture_struct(Codegen &gen);

public:

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::AllocaInst *llvm_allocate(Codegen &gen, const std::string &identifier) override;

    llvm::Value *llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_type() const override;

    void link(SymbolResolver &linker) override;

    void link(SymbolResolver &linker, VarInitStatement *stmnt) override;

    void link(SymbolResolver &linker, StructValue *value, const std::string &name) override;

    void link(SymbolResolver &linker, FunctionCall *call, unsigned int index) override;

    void link(SymbolResolver &linker, ReturnStatement *returnStmt) override;

    ValueType value_type() const override;

private:
    std::shared_ptr<FunctionType> func_type = nullptr;

};