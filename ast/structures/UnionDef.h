// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/UnionType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/LinkedType.h"

struct UnionDeclAttributes {

    AccessSpecifier specifier;

    bool is_comptime;

    bool is_direct_init;

    bool deprecated;

    bool anonymous;

};

class UnionDef : public ExtendableMembersContainerNode, public UnionType {
public:

    UnionDeclAttributes attrs;
    ASTNode* parent_node;
    SourceLocation location;
    LinkedType linked_type;

#ifdef COMPILER_BUILD
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    UnionDef(
        LocatedIdentifier identifier,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(std::move(identifier)), parent_node(parent_node), location(location),
        attrs(specifier, false, false, false, false), linked_type("", this, location) {

    }

    /**
    * get the name of node
    */
    inline LocatedIdentifier* get_located_id() {
        return &identifier;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::UnionDecl;
    }

    std::string union_name_str() final {
        return name_view().str();
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

    inline bool is_generic() {
        return !generic_params.empty();
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline bool is_anonymous() final {
        return attrs.anonymous;
    }

    inline void set_anonymous(bool value) {
        attrs.anonymous = value;
    }

    uint64_t byte_size(bool is64Bit) final {
        return UnionType::byte_size(is64Bit);
    }

    uint64_t byte_size(bool is64Bit, int16_t iteration) {
        auto prev = active_iteration;
        set_active_iteration(iteration);
        auto size = UnionType::byte_size(is64Bit);
        set_active_iteration(prev);
        return size;
    }

    VariablesContainer *as_variables_container() final {
        return this;
    }

    VariablesContainer *variables_container() final {
        return this;
    }

    VariablesContainer* copy_container(ASTAllocator &allocator) final;

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    const std::string ns_node_identifier() final {
        return name_str();
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    [[nodiscard]]
    BaseType * copy(ASTAllocator &allocator) const final;

    void declare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker) final;

    ASTNode *linked_node() final {
        return this;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Union;
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

    llvm::Type *llvm_type(Codegen &gen) final {
        return UnionType::llvm_type(gen);
    }

    llvm::Type *llvm_type(Codegen &gen, int16_t iteration) {
        auto prev = active_iteration;
        set_active_iteration(iteration);
        auto type = llvm_type(gen);
        set_active_iteration(prev);
        return type;
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final {
        return UnionType::llvm_chain_type(gen, values, index);
    }

    llvm::StructType *llvm_union_get_stored_type() final {
        return llvm_struct_type;
    }

    void llvm_union_type_store(llvm::StructType* type) final {
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