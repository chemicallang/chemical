// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ordered_map.h"
#include "ast/base/AccessSpecifier.h"

class Namespace : public AnnotableNode {
private:

    void declare_node(SymbolResolver& linker, ASTNode* node, const std::string& node_id);

    void declare_extended_in_linker(SymbolResolver& linker);

public:

    std::string name;
    std::vector<ASTNode*> nodes;
    tsl::ordered_map<std::string, ASTNode*> extended;
    Namespace* root = nullptr; // the root's namespace extended map contains pointers to all nodes
    ASTNode* parent_node;
    CSTToken* token;
    AccessSpecifier specifier;

    /**
     * constructor
     */
    Namespace(
        std::string name,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier = AccessSpecifier::Internal
    );

    CSTToken *cst_token() override {
        return token;
    }

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

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    ASTNode *child(const std::string &name) override;

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) override;

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) override;

    void code_gen_external_declare(Codegen &gen) override;

    void code_gen_destruct(Codegen &gen, Value *returnValue) override;

#endif

};