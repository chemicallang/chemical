// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "Scope.h"
#include "ast/base/Value.h"
#include <optional>

class IfStatement : public ASTNode {
public:

    /**
     * @brief Construct a new IfStatement object.
     *
     * @param condition The condition of the if statement.
     * @param ifBody The body of the if statement.
     * @param elseBody The body of the else statement (can be nullptr if there's no else part).
     */
    IfStatement(
            std::unique_ptr<Value> condition,
            Scope ifBody,
            std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs,
            std::optional<Scope> elseBody
    );

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool gen_last_block);

    void code_gen(Codegen &gen) override;

    void code_gen(Codegen &gen, std::vector<std::unique_ptr<ASTNode>> &nodes, unsigned int index) override;

#endif

    void interpret(InterpretScope &scope) override;

    std::string representation() const override;

    std::unique_ptr<Value> condition;
    Scope ifBody;
    std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs;
    std::optional<Scope> elseBody;
};