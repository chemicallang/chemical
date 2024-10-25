// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/StructType.h"

class UnnamedStruct : public BaseDefMember, public VariablesContainer, public StructType {
public:

    ASTNode* parent_node;
    SourceLocation location;
    AccessSpecifier specifier;

    UnnamedStruct(
        std::string name,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    );

    ASTNodeKind kind() final {
        return ASTNodeKind::UnnamedStruct;
    }

    SourceLocation encoded_location() override {
        return location;
    }

    BaseType* known_type() final {
        return this;
    }

    std::string get_runtime_name() final {
        return "";
    }

    VariablesContainer *variables_container() final {
        return this;
    }

    VariablesContainer *as_variables_container() final {
        return this;
    }

    int16_t get_generic_iteration() final {
        return 0;
    }

    bool get_is_const() final {
        // TODO allow user to mark unnamed structs const
        return false;
    }

    BaseDefMember* copy_member(ASTAllocator &allocator) final;

    VariablesContainer *copy_container(ASTAllocator& allocator) final;

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void redeclare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker) final;

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

    ASTNode *child(const std::string &name) final {
        return VariablesContainer::child_def_member(name);
    }

    uint64_t byte_size(bool is64Bit) final {
        return total_byte_size(is64Bit);
    }

    ASTNode *linked_node() final {
        return this;
    }

    BaseType* create_value_type(ASTAllocator &allocator) final;

    [[nodiscard]]
    BaseType* copy(ASTAllocator& allocator) const final;

#ifdef COMPILER_BUILD

    llvm::Type  *llvm_type(Codegen &gen) final {
        return StructType::llvm_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final {
        return StructType::llvm_chain_type(gen, values, index);
    }

    bool add_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const std::string &name
    ) final {
        return VariablesContainer::llvm_struct_child_index(gen, indexes, name);
    }

#endif

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Struct;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Struct;
    }

};