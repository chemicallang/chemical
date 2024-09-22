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

    BaseType* known_type() override {
        return this;
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

    BaseDefMember* copy_member(ASTAllocator &allocator) override;

    VariablesContainer *copy_container(ASTAllocator& allocator) override;

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void redeclare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    bool requires_copy_fn() {
        for(const auto& var : variables) {
            if(var.second->known_type()->requires_copy_fn()) {
                return true;
            }
        }
        return false;
    }

    bool requires_move_fn() {
        for(const auto& var : variables) {
            if(var.second->known_type()->requires_move_fn()) {
                return true;
            }
        }
        return false;
    }

    bool requires_clear_fn() {
        for(const auto& var : variables) {
            if(var.second->known_type()->requires_clear_fn()) {
                return true;
            }
        }
        return false;
    }

    bool requires_destructor() {
        for(const auto& var : variables) {
            if(var.second->known_type()->requires_destructor()) {
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

    ASTNode *linked_node() override {
        return this;
    }

    BaseType* create_value_type(ASTAllocator &allocator) override;

    [[nodiscard]]
    BaseType* copy(ASTAllocator& allocator) const override;

#ifdef COMPILER_BUILD

    llvm::Type  *llvm_type(Codegen &gen) override {
        return StructType::llvm_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override {
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