// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Scope : public ASTNode {
public:

    /**
     * empty constructor
     */
    Scope() {

    }

    /**
     * @brief Construct a new Scope object.
     * @param nodes All the ASTNode(s) present in the scope
     */
    Scope(std::vector<std::unique_ptr<ASTNode>> nodes);

    /**
     * move constructor
     * @param other
     */
    Scope(Scope &&other);

    void accept(Visitor &visitor) override;

    /**
     * a scope's declare_top_level will be called to link all the nodes
     */
    void declare_top_level(ASTLinker &linker) override;

    void declare_and_link(ASTLinker &linker) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void interpret(InterpretScope &scope) override;

    /**
     * function is supposed to implemented by other scopes
     * like loop scope, which can be stopped in the middle of the loop
     */
    virtual void stopInterpretOnce();

    std::string representation() const override;

    std::vector<std::unique_ptr<ASTNode>> nodes;

};