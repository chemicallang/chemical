// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/base/ExtendableAnnotableNode.h"
#include "ast/base/LocatedIdentifier.h"

struct TypealiasDeclAttributes {

    /**
     * the access specifier
     */
    AccessSpecifier specifier;

    /**
     * is comptime typealias
     */
    bool is_comptime = false;

    /**
     * is typealias deprecated
     */
    bool deprecated = false;

    /**
     * no mangle means don't prefix with module name
     */
    bool is_no_mangle = false;

};

class TypealiasStatement : public ExtendableNode {
public:

    TypealiasDeclAttributes attrs;
    // before equal
    LocatedIdentifier located_id;
    // after equal
    TypeLoc actual_type;
    /**
     * the generic type decl that is the generic parent of the typealias statement
     */
    GenericTypeDecl* generic_parent;
    /**
     * if this is a generic instantiation, this variable is set to reflect that
     */
    int generic_instantiation = -1;

    /**
     * constructor
     */
    TypealiasStatement(
            LocatedIdentifier identifier,
            TypeLoc actual_type,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableNode(ASTNodeKind::TypealiasStmt, parent_node, location), located_id(identifier),
        actual_type(actual_type), attrs(specifier, false, false) {

    }

    TypealiasStatement* shallow_copy(ASTAllocator& allocator) {
        const auto stmt = new (allocator.allocate<TypealiasStatement>()) TypealiasStatement(
                located_id,
                actual_type,
                parent(),
                encoded_location(),
                specifier()
        );
        stmt->attrs = attrs;
        return stmt;
    }

    TypealiasStatement* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<TypealiasStatement>()) TypealiasStatement(
            located_id,
            actual_type.copy(allocator),
            parent(),
            encoded_location(),
            specifier()
        );
        stmt->attrs = attrs;
        return stmt;
    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &located_id;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline AccessSpecifier specifier() {
        return attrs.specifier;
    }

    inline void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    inline bool is_no_mangle() {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool mangle) {
        attrs.is_no_mangle = mangle;
    }

    inline const chem::string_view& name_view() const {
        return located_id.identifier;
    }

    inline std::string name_str() const {
        return name_view().str();
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    uint64_t byte_size(bool is64Bit) final;

    BaseType* known_type() final;

    ASTNode* child(const chem::string_view &name) override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Type* llvm_param_type(Codegen &gen) override;

    void code_gen(Codegen &gen) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name) override {
        const auto linked = actual_type->linked_node();
        return linked != nullptr && linked->add_child_index(gen, indexes, name);
    }

#endif

};