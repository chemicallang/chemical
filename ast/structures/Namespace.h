// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ordered_map.h"

class Namespace : public AnnotableNode {
public:

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> nodes;
    tsl::ordered_map<std::string, ASTNode*> extended;
    Namespace* root = nullptr; // the root's namespace extended map contains pointers to all nodes
    ASTNode* parent_node;

    /**
     * constructor
     */
    Namespace(std::string name, ASTNode* parent_node);

    ASTNodeKind kind() override {
        return ASTNodeKind::NamespaceDecl;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    const std::string& ns_node_identifier() override {
        return name;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    ASTNode *child(const std::string &name) override;

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) override;

    void code_gen_destruct(Codegen &gen, Value *returnValue) override;

#endif

};