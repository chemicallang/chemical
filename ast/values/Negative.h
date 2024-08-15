// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a negative operator -value
class NegativeValue : public Value {
public:

    std::unique_ptr<Value> value;

    explicit NegativeValue(std::unique_ptr<Value> value) : value(std::move(value)) {}

    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

    uint64_t byte_size(bool is64Bit) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    bool primitive() override;

    Value* copy() override {
        return new NegativeValue(std::unique_ptr<Value>(value->copy()));
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    std::unique_ptr<BaseType> create_type() override;

    [[nodiscard]]
    ValueType value_type() const override {
        return value->value_type();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return value->type_kind();
    }

};