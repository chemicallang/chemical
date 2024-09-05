// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "MembersContainer.h"
#include <optional>
#include <map>
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/StructType.h"

class StructDefinition : public ExtendableMembersContainerNode, public StructType {
public:

    AccessSpecifier specifier;
    bool is_direct_init = false;
    ASTNode* parent_node;
    CSTToken* token;

#ifdef COMPILER_BUILD
    /**
     * the key here is the generic iteration, where as the value corresponds to
     * a llvm struct type, we create a struct type once, and then cache it
     */
    std::unordered_map<int16_t, llvm::StructType*> llvm_struct_types;
    llvm::GlobalVariable* vtable_pointer = nullptr;
#endif

    using MembersContainer::requires_destructor;

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            std::string name,
            ASTNode* parent_node,
            CSTToken* token,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

    ASTNodeKind kind() override {
        return ASTNodeKind::StructDecl;
    }

    int16_t get_generic_iteration() override {
        return active_iteration;
    }

    VariablesContainer *copy_container() override;

    ASTNode *linked_node() override {
        return this;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    CSTToken *cst_token() override {
        return token;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    VariablesContainer *as_variables_container() override {
        return this;
    }

    const std::string& ns_node_identifier() override {
        return name;
    }

    std::string struct_name() override {
        return name;
    }

    VariablesContainer *variables_container() override {
        return this;
    }

    bool is_exported_fast() {
        return specifier == AccessSpecifier::Public;
    }

    bool is_generic() {
        return !generic_params.empty();
    }

    void accept(Visitor *visitor) override;

    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    ASTNode *child(const std::string &name) override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    BaseType* known_type() override;

    [[nodiscard]]
    ValueType value_type() const override;

    uint64_t byte_size(bool is64Bit) override {
        return total_byte_size(is64Bit);
    }

    uint64_t byte_size(bool is64Bit, int16_t iteration) {
        auto prev = active_iteration;
        set_active_iteration(iteration);
        auto size = total_byte_size(is64Bit);
        set_active_iteration(prev);
        return size;
    }

    [[nodiscard]]
    BaseType *copy() const override;

#ifdef COMPILER_BUILD

    bool is_anonymous() override {
        return has_annotation(AnnotationKind::Anonymous);
    }

    llvm::StructType* llvm_stored_type() override;

    void llvm_store_type(llvm::StructType* type) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen, int16_t iteration);

    llvm::Type *llvm_param_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

    /**
     * will try to override the given function if there's an interface and it exists
     * in the inherited struct / interface, otherwise returns false
     */
    bool llvm_override(Codegen& gen, FunctionDeclaration* declaration);

    /**
     * generate code for all functions in this struct
     */
    void struct_func_gen(Codegen& gen, const std::vector<std::unique_ptr<FunctionDeclaration>>& funcs);

    /**
     * for the given struct iteration, we acquire all the function iterations and put them
     * in the llvm_struct types, this basically set's the given iteration so that when llvm_type is called
     * or llvm_value, it will consider the struct iteration
     */
    void acquire_function_iterations(int16_t iteration);

    void code_gen(Codegen &gen) override;

    void code_gen_generic(Codegen &gen) override;

    void code_gen_external_declare(Codegen &gen) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

#endif

};