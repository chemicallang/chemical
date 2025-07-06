// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/AnnotableNode.h"
#include <unordered_map>
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
    bool is_comptime = false;

    /**
     * compiler declarations are present inside the compiler, no need to import
     */
    bool is_compiler_decl = false;

    /**
     * is namespace deprecated
     */
    bool deprecated = false;

    /**
     * if it's anonymous, it's name wouldn't make it to the final runtime name
     */
    bool is_anonymous = false;

};

class Namespace : public ASTNode {
public:

    LocatedIdentifier identifier;
    std::vector<ASTNode*> nodes;
    std::unordered_map<chem::string_view, ASTNode*> extended;
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
        attrs(specifier, false, false, false, false)
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

    /**
     * get the name of the node
     */
    inline chem::string_view name_view() {
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

    inline bool is_anonymous() {
        return attrs.is_anonymous;
    }

    inline void set_anonymous(bool anonymous) {
        attrs.is_anonymous = anonymous;
    }

    void put_in_extended(std::unordered_map<chem::string_view, ASTNode*>& extended);

    /**
     * if the given namespace has a root namespace, it will extend it
     * basically putting its children into root namespace's extended map
     */
    void extend_root_namespace() {
        if(root) {
            put_in_extended(root->extended);
        }
    }

    void declare_node(SymbolResolver& linker, ASTNode* node, const chem::string_view& node_id);

    void declare_extended_in_linker(SymbolResolver& linker);

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