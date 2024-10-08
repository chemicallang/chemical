// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a not operator !value
class NotValue : public Value {
public:

    Value* value;
    CSTToken* token;

    explicit NotValue(Value* value, CSTToken* token) : value(value), token(token) {}

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::NotValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override;

    bool primitive() override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    NotValue* copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<NotValue>()) NotValue(value->copy(allocator), token);
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return value->value_type();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return value->type_kind();
    }

    BaseType* create_type(ASTAllocator &allocator) override;

//    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

};