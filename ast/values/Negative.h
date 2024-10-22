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

    Value* value;
    CSTToken* token;

    explicit NegativeValue(Value* value, CSTToken* token) : value(value), token(token) {}

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::NegativeValue;
    }

//    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

    uint64_t byte_size(bool is64Bit) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override;

    bool primitive() override;

    Value* copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<NegativeValue>()) NegativeValue(value->copy(allocator), token);
    }

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    BaseType* create_type(ASTAllocator &allocator) override;
//    std::unique_ptr<BaseType> create_type() override;

    [[nodiscard]]
    ValueType value_type() const override {
        return value->value_type();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return value->type_kind();
    }

};