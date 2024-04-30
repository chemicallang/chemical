// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class ReturnStatement : public ASTNode {
public:

    /**
     * @brief Construct a new ReturnStatement object.
     */
    ReturnStatement(std::optional<std::unique_ptr<Value>> value, FunctionDeclaration *declaration);

    void interpret(InterpretScope &scope) override;

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor &visitor) override;

    ReturnStatement *as_return() override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    std::string representation() const override;

    FunctionDeclaration *declaration;
    std::optional<std::unique_ptr<Value>> value;

};