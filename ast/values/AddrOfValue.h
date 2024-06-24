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

    AddrOfValue(std::unique_ptr<Value> value);

    uint64_t byte_size(bool is64Bit) const override {
        return is64Bit ? 8 : 4;
    }

    Value *copy() override;

    std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<PointerType>(value->create_type());
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ASTNode *linked_node() override {
        return value->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    ValueType value_type() const override {
        return ValueType::Pointer;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Pointer;
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    std::unique_ptr<Value> value;
};