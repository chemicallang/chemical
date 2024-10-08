// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/UnionType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/LinkedType.h"

class UnionDef : public ExtendableMembersContainerNode, public UnionType {
public:

    ASTNode* parent_node;
    CSTToken* token;
    bool is_direct_init = false;
    AccessSpecifier specifier;
    LinkedType linked_type;

#ifdef COMPILER_BUILD
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    UnionDef(
        std::string name,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier = AccessSpecifier::Internal
    );

    CSTToken* cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::UnionDecl;
    }

    std::string union_name() override {
        return name;
    }

    bool is_exported_fast() {
        return specifier == AccessSpecifier::Public;
    }

    inline bool is_generic() {
        return !generic_params.empty();
    }

    uint64_t byte_size(bool is64Bit) override {
        return UnionType::byte_size(is64Bit);
    }

    uint64_t byte_size(bool is64Bit, int16_t iteration) {
        auto prev = active_iteration;
        set_active_iteration(iteration);
        auto size = UnionType::byte_size(is64Bit);
        set_active_iteration(prev);
        return size;
    }

    VariablesContainer *as_variables_container() override {
        return this;
    }

    VariablesContainer *variables_container() override {
        return this;
    }

    VariablesContainer* copy_container(ASTAllocator &allocator) override;

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

    BaseType* create_value_type(ASTAllocator& allocator) override;

    BaseType* known_type() override;

    [[nodiscard]]
    BaseType * copy(ASTAllocator &allocator) const override;

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    ASTNode *linked_node() override {
        return this;
    }

    bool is_anonymous() override {
        return has_annotation(AnnotationKind::Anonymous);
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

    void code_gen_declare(Codegen &gen) override {
        code_gen(gen, true);
    }

    void code_gen(Codegen &gen) override {
        code_gen(gen, false);
    }

    void code_gen_external_declare(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override {
        return UnionType::llvm_type(gen);
    }

    llvm::Type *llvm_type(Codegen &gen, int16_t iteration) {
        auto prev = active_iteration;
        set_active_iteration(iteration);
        auto type = llvm_type(gen);
        set_active_iteration(prev);
        return type;
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override {
        return UnionType::llvm_chain_type(gen, values, index);
    }

    llvm::StructType *llvm_union_get_stored_type() override {
        return llvm_struct_type;
    }

    void llvm_union_type_store(llvm::StructType* type) override {
        llvm_struct_type = type;
    }

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &name
    ) override {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};