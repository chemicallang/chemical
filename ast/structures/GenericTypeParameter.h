// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class GenericTypeParameter : public ASTNode {
public:

    std::string identifier;
    BaseType* def_type;
    std::vector<BaseType*> usage;
    int16_t active_iteration = -1; // <-- index of active type in usage vector
    ASTNode* parent_node;
    unsigned param_index = 0; // <-- index in the generic type parameters
    CSTToken* token;

    /**
     * constructor
     */
    GenericTypeParameter(
        std::string identifier,
        BaseType* def_type,
        ASTNode* parent_node,
        unsigned param_index,
        CSTToken* token
    );

    CSTToken* cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::GenericTypeParam;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_and_link(SymbolResolver &linker) override;

    void register_usage(ASTAllocator& allocator, BaseType* type);

    BaseType* create_value_type(ASTAllocator& allocator) override {
        return usage[active_iteration]->copy(allocator);
    }

    [[nodiscard]]
    ValueType value_type() const override {
        if(active_iteration == -1) {
            return ValueType::Unknown;
        } else {
            return usage[active_iteration]->value_type();
        }
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        if(active_iteration == -1) {
            return BaseTypeKind::Unknown;
        } else {
            return usage[active_iteration]->kind();
        }
    }

    BaseType *known_type() override {
        if(active_iteration == -1) {
            return nullptr;
        } else {
            return usage[active_iteration];
        }
    }

    ASTNode *child(const std::string &name) override {
        return usage[active_iteration]->linked_node()->child(name);
    }

    int child_index(const std::string &name) override {
        return usage[active_iteration]->linked_node()->child_index(name);
    }

    ASTNode* child(int index) override {
        return usage[active_iteration]->linked_node()->child(index);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_param_type(Codegen &gen) override {
        return usage[active_iteration]->llvm_param_type(gen);
    }

    llvm::Type *llvm_type(Codegen &gen) override {
        return usage[active_iteration]->llvm_type(gen);
    }

    llvm::FunctionType *llvm_func_type(Codegen &gen) override {
        return usage[active_iteration]->llvm_func_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override {
        return usage[active_iteration]->llvm_chain_type(gen, values, index);
    }

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override {
        return usage[active_iteration]->linked_node()->add_child_index(gen, indexes, name);
    }

#endif

    ASTNode *parent() override {
        return parent_node;
    }

};