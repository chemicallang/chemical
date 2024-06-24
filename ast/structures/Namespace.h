// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class Namespace : public ASTNode {
public:

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> nodes;
    std::unordered_map<std::string, ASTNode*> extended;

    /**
     * constructor
     */
    explicit Namespace(std::string name);

    std::string ns_node_identifier() override {
        return name;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    ASTNode *child(const std::string &name) override;

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) override;

    void code_gen_destruct(Codegen &gen, Value *returnValue) override;

#endif

};