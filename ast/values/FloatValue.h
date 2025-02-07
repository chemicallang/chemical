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
    SourceLocation location;

    /**
     * @brief Construct a new FloatValue object.
     *
     * @param value The floating-point value.
     */
    explicit FloatValue(float value, SourceLocation location) : value(value), location(location) {}

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::Float;
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &FloatType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &FloatType::instance;
    }

    uint64_t byte_size(bool is64Bit) {
        return 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<FloatType>()) FloatType(location);
    }

    FloatValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<FloatValue>()) FloatValue(value, location);
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Float;
    }

};