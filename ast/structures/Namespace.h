// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ordered_map.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/LocatedIdentifier.h"

struct NamespaceDeclAttributes {

    /**
     * the access specifier for the namespace decl
     */
    AccessSpecifier specifier;

    /**
     * is the whole namespace comptime
     */
    bool is_comptime;

    /**
     * is namespace deprecated
     */
    bool deprecated;

};

class Namespace : public AnnotableNode {
private:

    void declare_node(SymbolResolver& linker, ASTNode* node, const chem::string_view& node_id);

    void declare_extended_in_linker(SymbolResolver& linker);

public:

    LocatedIdentifier identifier;
    std::vector<ASTNode*> nodes;
    tsl::ordered_map<chem::string_view, ASTNode*> extended;
    Namespace* root = nullptr; // the root's namespace extended map contains pointers to all nodes
    ASTNode* parent_node;
    SourceLocation location;
    NamespaceDeclAttributes attrs;

    /**
     * constructor
     */
    Namespace(
        LocatedIdentifier identifier,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    );

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &identifier;
    }

    /**
     * get the name of the node
     */
    inline chem::string_view name() {
        return identifier.identifier;
    }

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

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
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

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    ASTNode *child(const chem::string_view &name) final;

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) final;

    void code_gen(Codegen &gen) final;

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) final;

    void code_gen_external_declare(Codegen &gen) final;

    void code_gen_destruct(Codegen &gen, Value *returnValue) final;

#endif

};