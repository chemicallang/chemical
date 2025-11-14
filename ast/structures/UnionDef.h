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

    bool is_copy = false;

    bool is_no_mangle = false;

};

class UnionDef : public ExtendableMembersContainerNode {
public:

    UnionDeclAttributes attrs;
    LinkedType linked_type;

#ifdef COMPILER_BUILD
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    UnionDef(
        LocatedIdentifier identifier,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::UnionDecl, parent_node, location),
        attrs(specifier, false, false, false, false), linked_type(this) {

    }

    /**
    * get the name of node
    */
    inline LocatedIdentifier* get_located_id() {
        return &identifier;
    }


//    std::string union_name_str() final {
//        return name_view().str();
//    }

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

    inline bool is_shallow_copyable() {
        return attrs.is_copy;
    }

    inline void set_shallow_copyable(bool value) {
        attrs.is_copy = value;
    }

    inline bool is_no_mangle() {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool no_mangle) {
        attrs.is_no_mangle = no_mangle;
    }

    UnionDef* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<UnionDef>()) UnionDef(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
    }

    uint64_t byte_size(bool is64Bit) final {
        return largest_member_byte_size(is64Bit);
    }

    VariablesContainer *as_variables_container() final {
        return this;
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

    void func_gen(Codegen &gen, bool declare);

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

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    inline llvm::StructType *llvm_union_get_stored_type() {
        return llvm_struct_type;
    }

    inline void llvm_union_type_store(llvm::StructType* type) {
        llvm_struct_type = type;
    }

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const chem::string_view &name
    ) final {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};