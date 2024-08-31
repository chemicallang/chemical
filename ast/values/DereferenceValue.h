// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"

class DereferenceValue : public Value {
public:

    std::unique_ptr<Value> value;
    CSTToken* token;

    explicit DereferenceValue(std::unique_ptr<Value> value, CSTToken* token);

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::DereferenceValue;
    }

    uint64_t byte_size(bool is64Bit) override {
        return value->byte_size(is64Bit);
    }

    DereferenceValue *as_deref_value() override {
        return this;
    }

    DereferenceValue *copy() override;

    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

    std::unique_ptr<BaseType> create_type() override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen& gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

};