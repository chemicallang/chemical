// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class GenericTypeParameter : public ASTNode {
public:

    std::string identifier;
    BaseType* at_least_type;
    BaseType* def_type;
    std::vector<BaseType*> usage;
    int16_t active_iteration = -1; // <-- index of active type in usage vector
    ASTNode* parent_node;
    unsigned param_index = 0; // <-- index in the generic type parameters
    SourceLocation location;

    /**
     * constructor
     */
    GenericTypeParameter(
        std::string identifier,
        BaseType* at_least_type,
        BaseType* def_type,
        ASTNode* parent_node,
        unsigned param_index,
        SourceLocation location
    );

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::GenericTypeParam;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void declare_only(SymbolResolver& linker);

    void declare_and_link(SymbolResolver &linker) final;

    void register_usage(ASTAllocator& allocator, BaseType* type);

    BaseType* create_value_type(ASTAllocator& allocator) final {
        return usage[active_iteration]->copy(allocator);
    }

    [[nodiscard]]
    ValueType value_type() const final {
        if(active_iteration == -1) {
            return ValueType::Unknown;
        } else {
            return usage[active_iteration]->value_type();
        }
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        if(active_iteration == -1) {
            return BaseTypeKind::Unknown;
        } else {
            return usage[active_iteration]->kind();
        }
    }

    BaseType *known_type() final {
        if(active_iteration == -1) {
            return nullptr;
        } else {
            return usage[active_iteration];
        }
    }

    ASTNode* usage_linked() {
        return active_iteration > -1 ? usage[active_iteration]->linked_node() : nullptr;
    }

    ASTNode *child(const std::string &name) final {
        const auto linked = usage_linked();
        return linked ? linked->child(name) : nullptr;
    }

    int child_index(const std::string &name) final {
        const auto linked = usage_linked();
        return linked ? linked->child_index(name) : -1;
    }

    ASTNode* child(int index) final {
        const auto linked = usage_linked();
        return linked ? linked->child(index) : nullptr;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_param_type(Codegen &gen) final {
        return usage[active_iteration]->llvm_param_type(gen);
    }

    llvm::Type *llvm_type(Codegen &gen) final {
        return usage[active_iteration]->llvm_type(gen);
    }

    llvm::FunctionType *llvm_func_type(Codegen &gen) final {
        return usage[active_iteration]->llvm_func_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final {
        return usage[active_iteration]->llvm_chain_type(gen, values, index);
    }

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final {
        return usage[active_iteration]->linked_node()->add_child_index(gen, indexes, name);
    }

#endif

    ASTNode *parent() final {
        return parent_node;
    }

};