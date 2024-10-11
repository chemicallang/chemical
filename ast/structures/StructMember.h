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
        if(defValue) return defValue;
        return nullptr;
    }

    BaseDefMember *copy_member(ASTAllocator& allocator) override;

    bool get_is_const() override {
        return is_const;
    }

    void accept(Visitor *visitor) override;

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    ASTNode *child(const std::string &name) override;

    Value *holding_value() override {
        return defValue ? defValue : nullptr;
    }

    BaseType *known_type() override {
        return type;
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Type* llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override;

    llvm::FunctionType* llvm_func_type(Codegen &gen) override;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) override;

    [[nodiscard]]
    ValueType value_type() const override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

};