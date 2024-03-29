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
    Scope(std::vector<std::unique_ptr<ASTNode>> nodes) : nodes(std::move(nodes)) {}

    /**
     * move constructor
     * @param other
     */
    Scope(Scope &&other) : nodes(std::move(other.nodes)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    /**
     * a scope's declare_top_level will be called to link all the nodes
     */
    void declare_top_level(ASTLinker &linker) override {
        for(const auto& node : nodes) {
            node->declare_top_level(linker);
        }
        for(const auto& node : nodes) {
            node->declare_and_link(linker);
        }
    }

    void declare_and_link(ASTLinker &linker) override {
        for(const auto& node : nodes) {
            node->declare_and_link(linker);
        }
        for(const auto& node : nodes) {
            node->undeclare_on_scope_end(linker);
        }
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override {
        for(const auto& node : nodes) {
            node->code_gen(gen);
        }
    }
#endif

    void interpret(InterpretScope &scope) override;

    /**
     * function is supposed to implemented by other scopes
     * like loop scope, which can be stopped in the middle of the loop
     */
    virtual void stopInterpretOnce() {

    }

    std::string representation() const override {
        std::string rep;
        int i = 0;
        while (i < nodes.size()) {
            rep.append(nodes[i]->representation());
            if (i != nodes.size() - 1) {
                rep.append(1, '\n');
            }
            i++;
        }
        return rep;
    }

    std::vector<std::unique_ptr<ASTNode>> nodes;

};