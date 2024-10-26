// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/values/IntNumValue.h"
#include "ast/types/CharType.h"

/**
 * @brief Class representing a character value.
 */
class CharValue : public IntNumValue {
public:

    char value; ///< The character value.
    SourceLocation location;

    /**
     * @brief Construct a new CharValue object.
     *
     * @param value The character value.
     */
    CharValue(char value, SourceLocation location) : value(value), location(location) {

    }

    unsigned int get_num_bits() final {
        return 8;
    }

    [[nodiscard]]
    int64_t get_num_value() const final {
        return value;
    }

    bool is_unsigned() final {
        return false;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::Char;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    BaseType* known_type() final {
        return (BaseType*) &CharType::instance;
    }

    BaseType* create_type(ASTAllocator& allocator) final {
        return new (allocator.allocate<CharType>()) CharType(location);
    }

    CharValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<CharValue>()) CharValue(value, location);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Char;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::IntN;
    }

};