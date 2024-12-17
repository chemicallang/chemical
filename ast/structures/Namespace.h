// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ordered_map.h"
#include "ast/base/AccessSpecifier.h"

struct NamespaceDeclAttributes {

    /**
     * the access specifier for the namespace decl
     */
    AccessSpecifier specifier;

    /**
     * is the whole namespace comptime
     */
    bool is_comptime;

};

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
    SourceLocation location;
    NamespaceDeclAttributes attrs;

    /**
     * constructor
     */
    Namespace(
        std::string name,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    );

    inline AccessSpecifier specifier() {
        return attrs.specifier;
    }

    inline void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) {
        attrs.is_comptime = value;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::NamespaceDecl;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    const std::string ns_node_identifier() final {
        return name;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker) final;

    ASTNode *child(const std::string &name) final;

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) final;

    void code_gen(Codegen &gen) final;

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) final;

    void code_gen_external_declare(Codegen &gen) final;

    void code_gen_destruct(Codegen &gen, Value *returnValue) final;

#endif

};