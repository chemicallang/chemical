// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/FloatType.h"

/**
 * @brief Class representing a floating-point value.
 */
class FloatValue : public Value {
public:

    float value; ///< The floating-point value.
    CSTToken* token;

    /**
     * @brief Construct a new FloatValue object.
     *
     * @param value The floating-point value.
     */
    explicit FloatValue(float value, CSTToken* token) : value(value), token(token) {}

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::Float;
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { (BaseType*) &FloatType::instance, false };
//    }

    BaseType* known_type() override {
        return (BaseType*) &FloatType::instance;
    }

    uint64_t byte_size(bool is64Bit) {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    BaseType* create_type(ASTAllocator &allocator) override {
        return new (allocator.allocate<FloatType>()) FloatType(nullptr);
    }

    FloatValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<FloatValue>()) FloatValue(value, token);
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Float;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Float;
    }

};