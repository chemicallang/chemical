// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/types/IntNType.h"
#include "IntNumValue.h"
#include "ast/types/IntType.h"

/**
 * @brief Class representing an integer value.
 */
class IntValue : public IntNumValue {
public:

    int value; ///< The integer value.
    CSTToken* token;

    /**
     * constructor
     */
    explicit IntValue(int value, CSTToken* token) : value(value), token(token) {}

    CSTToken *cst_token() final {
        return token;
    }

    ValueKind val_kind() final {
        return ValueKind::Int;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    unsigned int get_num_bits() final {
        return 32;
    }

    [[nodiscard]]
    int64_t get_num_value() const final {
        return value;
    }

    IntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<IntValue>()) IntValue(value, token);
    }

    bool is_unsigned() final {
        return false;
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator& allocator) final {
        return new (allocator.allocate<IntType>()) IntType(nullptr);
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &IntType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &IntType::instance;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Int;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::IntN;
    }

};