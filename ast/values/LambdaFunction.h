// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/Scope.h"
#include "ast/types/FunctionType.h"

class FunctionDeclaration;

/**
 * @brief Class representing an integer value.
 */
class LambdaFunction : public Value {
public:

    std::vector<std::string> captureList;
    std::vector<std::unique_ptr<FunctionParam>> params;
    bool isVariadic;
    Scope scope;

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    LambdaFunction(
        std::vector<std::string> captureList,
        std::vector<std::unique_ptr<FunctionParam>> params,
        bool isVariadic,
        Scope scope
    );

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    std::string representation() const override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    void link(SymbolResolver &linker, FunctionCall *call, unsigned int index) override;

    void link(SymbolResolver &linker, ReturnStatement *returnStmt) override;

    ValueType value_type() const override;

private:
    std::shared_ptr<FunctionType> func_type = nullptr;

};