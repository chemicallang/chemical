// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Scope : public ASTNode {
public:

//    /**
//     * populated when required, for example when generating code, each node in nodes is traversed
//     * traverse function also takes a Scope*, the child scope which is a node, saves it when found
//     */
//    Scope* parent = nullptr;
//    int index_in_parent = -1;

    /**
     * empty constructor
     */
    Scope() = default;

    /**
     * @brief Construct a new Scope object.
     * @param nodes All the ASTNode(s) present in the scope
     */
    explicit Scope(std::vector<std::unique_ptr<ASTNode>> nodes);

    /**
     * move constructor
     */
    Scope(Scope &&other) noexcept;

    void accept(Visitor *visitor) override;

    /**
     * a scope's declare_top_level will be called to link all the nodes
     */
    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

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