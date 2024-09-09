// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"

class AddrOfValue : public Value {
public:

    std::unique_ptr<Value> value;
    CSTToken* token;
    PointerType _ptr_type;

    explicit AddrOfValue(std::unique_ptr<Value> value, CSTToken* token);

    CSTToken* cst_token() override{
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::AddrOfValue;
    }

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    AddrOfValue *copy() override;

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { new PointerType(value->create_type(), nullptr) };
    }

    std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<PointerType>(value->create_type(), nullptr);
    }

    BaseType* known_type() override {
        return &_ptr_type;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ASTNode *linked_node() override {
        return value->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    ValueType value_type() const override {
        return ValueType::Pointer;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Pointer;
    }

    bool link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

};