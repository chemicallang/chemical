// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

class CastedValue : public Value {
public:

    std::unique_ptr<Value> value;
    std::unique_ptr<BaseType> type;
    CSTToken* token;

    CastedValue(std::unique_ptr<Value> value, std::unique_ptr<BaseType> type, CSTToken* token);

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::CastedValue;
    }

    CastedValue *copy() override;

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { type.get(), false };
    }

    BaseType* known_type() override {
        return type.get();
    }

    std::unique_ptr<BaseType> create_type() override {
        return std::unique_ptr<BaseType>(type->copy());
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    ASTNode *linked_node() override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    [[nodiscard]]
    ValueType value_type() const override {
        return type->value_type();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return type->kind();
    }

};