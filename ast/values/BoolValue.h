// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/BoolType.h"

class BoolValue : public Value {
public:

    bool value;
    CSTToken* token;

    /**
     * @brief Construct a new CharValue object.
     *
     * @param value The character value.
     */
    explicit BoolValue(bool value, CSTToken* token) : value(value), token(token) {

    }

    ValueKind val_kind() override {
        return ValueKind::Bool;
    }

    CSTToken *cst_token() override {
        return token;
    }

    BoolValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<BoolValue>()) BoolValue(value, token);
    }

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { (BaseType*) &BoolType::instance, false };
//    }

    BaseType* known_type() override {
        return (BaseType*) &BoolType::instance;
    }

    BaseType* create_type(ASTAllocator& allocator) override {
        return new (allocator.allocate<BoolType>()) BoolType(nullptr);
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Bool;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Bool;
    }

};