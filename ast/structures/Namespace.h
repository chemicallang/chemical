// Copyright (c) Chemical Language Foundation 2025.

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
     * compiler declarations are present inside the compiler, no need to import
     */
    bool is_compiler_decl;

    /**
     * is namespace deprecated
     */
    bool deprecated;

};

class Namespace : public ASTNode {
private:

    void declare_node(SymbolResolver& linker, ASTNode* node, const chem::string_view& node_id);

    void declare_extended_in_linker(SymbolResolver& linker);

public:

    LocatedIdentifier identifier;
    std::vector<ASTNode*> nodes;
    tsl::ordered_map<chem::string_view, ASTNode*> extended;
    Namespace* root = nullptr; // the root's namespace extended map contains pointers to all nodes
    NamespaceDeclAttributes attrs;

    /**
     * constructor
     */
    Namespace(
        LocatedIdentifier identifier,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ASTNode(ASTNodeKind::NamespaceDecl, parent_node, location), identifier(identifier),
        attrs(specifier, false, false)
    {

    }

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

    inline bool is_compiler_decl() {
        return attrs.is_compiler_decl;
    }

    inline void set_compiler_decl(bool value) {
        attrs.is_comptime = value;
        attrs.is_compiler_decl = value;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }



    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void link_signature(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    ASTNode *child(const chem::string_view &name) final;

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) final;

    void code_gen(Codegen &gen) final;

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) final;

    void code_gen_external_declare(Codegen &gen) final;

#endif

};