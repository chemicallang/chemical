// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"

class FunctionDeclaration;

/**
 * @brief Class representing an integer value.
 */
class LambdaFunction : public Value {
public:

    std::unique_ptr<FunctionDeclaration> decl;

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    LambdaFunction(std::unique_ptr<FunctionDeclaration> decl);

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    std::string representation() const override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    ValueType value_type() const override;

};