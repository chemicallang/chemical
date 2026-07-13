// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/UnionType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/LinkedType.h"

struct UnionDeclAttributes {

    AccessSpecifier specifier;

    bool is_comptime = false;

    bool is_direct_init = false;

    bool deprecated = false;

    bool anonymous = false;

    bool is_no_mangle = false;
    
    /**
     * are bodies of functions inside this struct retained across modules
     */
    bool retained_body = false;

};

class UnionDef : public ExtendableMembersContainerNode {
public:

    UnionDeclAttributes attrs;
    LinkedType linked_type;

    UnionDef(
        chem::string_view identifier,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::UnionDecl, parent_node, location),
        attrs(specifier, false, false, false, false), linked_type(this) {

    }

    inline AccessSpecifier specifier() {
        return attrs.specifier;
    }

    inline void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool is_deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline bool is_anonymous() {
        return attrs.anonymous;
    }

    inline void set_anonymous(bool value) {
        attrs.anonymous = value;
    }

    inline bool is_no_mangle() {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool no_mangle) {
        attrs.is_no_mangle = no_mangle;
    }

    inline bool is_body_retained() const noexcept {
        return attrs.retained_body;
    }

    void set_is_body_retained(bool value) noexcept {
        attrs.retained_body = value;
    }

    UnionDef* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<UnionDef>()) UnionDef(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
    }

    uint64_t byte_size(const TargetData& target) final {
        return largest_member_byte_size(target);
    }

    inline BaseType* known_type() {
        return &linked_type;
    }

#ifdef COMPILER_BUILD

    /**
     * responsible for generating code for a single function in a union decl
     * read the documentation in this decl
     */
    void code_gen_function_declare(Codegen &gen, FunctionDeclaration* decl);

    /**
     * responsible for generating code for a single function in a union decl
     * read the documentation in this decl
     */
    void code_gen_function_body(Codegen &gen, FunctionDeclaration* decl);

    void code_gen(Codegen &gen, bool declare);

    void code_gen_declare(Codegen &gen) final {
        code_gen(gen, true);
    }

    void code_gen(Codegen &gen) final {
        code_gen(gen, false);
    }

    void code_gen_external_declare(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<Value*> &values, unsigned int index) final;

    llvm::StructType *llvm_get_stored_type(Codegen& gen);

    void llvm_store_type(Codegen& gen, llvm::StructType* type);

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const chem::string_view &name
    ) final {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};