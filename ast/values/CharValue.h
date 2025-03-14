// Copyright (c) Chemical Language Foundation 2025.

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

    /**
     * constructor
     */
    constexpr CharValue(
        char value,
        SourceLocation location
    ) : IntNumValue(ValueKind::Char, location), value(value) {

    }

    unsigned int get_num_bits() final {
        return 8;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

    bool is_unsigned() final {
        return false;
    }


    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    BaseType* known_type() final {
        return (BaseType*) &CharType::instance;
    }

    BaseType* create_type(ASTAllocator& allocator) final {
        return new (allocator.allocate<CharType>()) CharType(encoded_location());
    }

    CharValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<CharValue>()) CharValue(value, encoded_location());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};