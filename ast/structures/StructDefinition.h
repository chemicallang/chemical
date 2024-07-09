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
#include "ast/types/ReferencedType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/StructType.h"

class StructDefinition : public ExtendableMembersContainerNode, public StructType {
public:

    std::optional<std::unique_ptr<ReferencedType>> overrides;
    ASTNode* parent_node;

#ifdef COMPILER_BUILD
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            std::string name,
            const std::optional<std::string>& overrides,
            ASTNode* parent_node
    );

    VariablesContainer *copy_container() override;

    ASTNode *linked_node() override {
        return this;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    VariablesContainer *as_variables_container() override {
        return this;
    }

    std::string ns_node_identifier() override {
        return name;
    }

    std::string struct_name() override {
        return name;
    }

    VariablesContainer *variables_container() override {
        return this;
    }

    void accept(Visitor *visitor) override;

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    StructDefinition *as_struct_def() override;

    ASTNode *child(const std::string &name) override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    ValueType value_type() const override;

    uint64_t byte_size(bool is64Bit) override {
        return total_byte_size(is64Bit);
    }

    FunctionDeclaration* create_destructor();

    BaseType *copy() const override;

#ifdef COMPILER_BUILD

    bool is_anonymous() override {
        return has_annotation(AnnotationKind::Anonymous);
    }

    llvm::StructType* llvm_stored_type() override {
        return llvm_struct_type;
    }

    void llvm_store_type(llvm::StructType* type) override {
        llvm_struct_type = type;
    }

    llvm::Type *llvm_type(Codegen &gen) override {
        return StructType::llvm_type(gen);
    }

    llvm::Type *llvm_param_type(Codegen &gen) override {
        return StructType::llvm_param_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned int index) override {
        return StructType::llvm_chain_type(gen, values, index);
    }

    void code_gen(Codegen &gen) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

#endif

};