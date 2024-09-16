// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/StructType.h"

class UnnamedStruct : public BaseDefMember, public VariablesContainer, public StructType {
public:

    ASTNode* parent_node;
    CSTToken* token;
    AccessSpecifier specifier;

    UnnamedStruct(
        std::string name,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier = AccessSpecifier::Internal
    );

    ASTNodeKind kind() override {
        return ASTNodeKind::UnnamedStruct;
    }

    CSTToken *cst_token() override {
        return token;
    }

    std::string get_runtime_name() override {
        return "";
    }

    VariablesContainer *variables_container() override {
        return this;
    }

    VariablesContainer *as_variables_container() override {
        return this;
    }

    int16_t get_generic_iteration() override {
        return 0;
    }

    BaseDefMember *copy_member() override;

    VariablesContainer *copy_container() override;

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void redeclare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    bool requires_copy_fn() override {
        for(const auto& var : variables) {
            if(var.second->requires_copy_fn()) {
                return true;
            }
        }
        return false;
    }

    bool requires_move_fn() override {
        for(const auto& var : variables) {
            if(var.second->requires_move_fn()) {
                return true;
            }
        }
        return false;
    }

    bool requires_clear_fn() override {
        for(const auto& var : variables) {
            if(var.second->requires_clear_fn()) {
                return true;
            }
        }
        return false;
    }

    bool requires_destructor() override {
        for(const auto& var : variables) {
            if(var.second->requires_destructor()) {
                return true;
            }
        }
        return false;
    }

    ASTNode *child(const std::string &name) override {
        return VariablesContainer::child_def_member(name);
    }

    uint64_t byte_size(bool is64Bit) override {
        return total_byte_size(is64Bit);
    }

    hybrid_ptr<BaseType> get_value_type() override;

    ASTNode *linked_node() override {
        return this;
    }

    [[nodiscard]]
    BaseType *copy() const override;

#ifdef COMPILER_BUILD

    llvm::Type  *llvm_type(Codegen &gen) override {
        return StructType::llvm_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override {
        return StructType::llvm_chain_type(gen, values, index);
    }

    bool add_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const std::string &name
    ) override {
        return VariablesContainer::llvm_struct_child_index(gen, indexes, name);
    }

#endif

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Struct;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Struct;
    }

};