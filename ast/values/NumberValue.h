// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/types/IntNType.h"
#include "IntNumValue.h"
#include "ast/types/IntType.h"
#include "ast/base/Value.h"

/**
 * @brief Class representing a number value integer / long
 *
 * suppose if user writes a function that returns long, parsing a int32 value would lose information
 * we could parse every value as a bigint value and return as appropriate type
 *
 * but it must link with the type it must return as, so this value is used, which automatically links
 * and returns as appropriate type automatically
 */
class NumberValue : public IntNumValue {
public:

    uint64_t value;
    BaseType* linked_type = nullptr;

    /**
     * constructor
     */
    constexpr NumberValue(
        uint64_t value,
        SourceLocation location
    ) : IntNumValue(ValueKind::NumberValue, location), value(value) {}

    bool link(SymbolResolver &linker, BaseType *type);

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) final {
        return link(linker, type);
    }

    void relink_after_generic(SymbolResolver &linker, BaseType *expected_type) final {
        link(linker, expected_type);
    }

    unsigned int get_num_bits() final;

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

    NumberValue *copy(ASTAllocator& allocator) final {
        auto copy = new (allocator.allocate<NumberValue>()) NumberValue(value, encoded_location());
        if(linked_type) {
            copy->linked_type = (IntNType *) linked_type->copy(allocator);
        }
        return copy;
    }

    bool is_unsigned() final;

    Value* evaluated_value(InterpretScope &scope) override;

//    hybrid_ptr<BaseType> get_base_type() final {
//        if(!linked_type) {
//            return hybrid_ptr<BaseType> { (BaseType*) &IntType::instance, false };
//        }
//        return hybrid_ptr<BaseType> { linked_type.get(), false };
//    }

    BaseType* known_type() final;

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        if(linked_type) {
            return linked_type->copy(allocator);
        } else {
            return new (allocator.allocate<IntType>()) IntType(encoded_location());
        }
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::IntN;
    }

};