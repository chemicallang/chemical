// Copyright (c) Qinetik 2024.

#pragma once

#include <map>
#include "ast/base/Value.h"
#include "BaseDefMember.h"

class StructMember : public BaseDefMember {
public:

    bool is_const;
    BaseType* type;
    Value* defValue;
    ASTNode* parent_node;
    CSTToken* token;
    AccessSpecifier specifier;

    StructMember(
            std::string name,
            BaseType* type,
            Value* defValue,
            ASTNode* parent_node,
            CSTToken* token,
            bool is_const = false,
            AccessSpecifier specifier = AccessSpecifier::Public
    );

    ASTNodeKind kind() override {
        return ASTNodeKind::StructMember;
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

    Value *default_value() override {
        if(defValue) return defValue.get();
        return nullptr;
    }

    BaseDefMember *copy_member() override;

    void accept(Visitor *visitor) override;

    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    ASTNode *child(const std::string &name) override;

    bool requires_destructor() override;

    bool requires_clear_fn() override;

    bool requires_move_fn() override;

    bool requires_copy_fn() override;

    Value *holding_value() override {
        return defValue ? defValue.get() : nullptr;
    }

    BaseType *known_type() override {
        return type.get();
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Type* llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

    llvm::FunctionType* llvm_func_type(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

};