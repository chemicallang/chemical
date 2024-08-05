// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class GenericTypeParameter : public ASTNode {
public:

    std::string identifier;
    std::unique_ptr<BaseType> def_type;
    std::vector<BaseType*> usage;
    int16_t active_iteration = -1; // <-- index of active type in usage vector
    ASTNode* parent_node;

    /**
     * constructor
     */
    GenericTypeParameter(
        std::string identifier,
        std::unique_ptr<BaseType> def_type,
        ASTNode* parent_node
    );

    void declare_and_link(SymbolResolver &linker) override;

    void register_usage(BaseType* type);

    hybrid_ptr<BaseType> get_value_type() override {
        return hybrid_ptr<BaseType> { usage[active_iteration], false };
    }

    std::unique_ptr<BaseType> create_value_type() override {
        return std::unique_ptr<BaseType>(usage[active_iteration]->copy());
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

    BaseType *holding_value_type() override {
        return usage[active_iteration];
    }

    GenericTypeParameter *as_generic_type_param() override {
        return this;
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

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override {
        return usage[active_iteration]->llvm_chain_type(gen, values, index);
    }

#endif

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        // do nothing
    }

};