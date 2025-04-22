// Copyright (c) Chemical Language Foundation 2025.

#include "IntNumValue.h"
#include "ast/types/ShortType.h"

class ShortValue : public IntNumValue {
public:

    short value;

    /**
     * constructor
     */
    constexpr ShortValue(
        short value,
        SourceLocation location
    ) : IntNumValue(ValueKind::Short, location), value(value) {

    }

    BaseType* known_type() final {
        return (BaseType*) &ShortType::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    ShortValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<ShortValue>()) ShortValue(value, encoded_location());
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<ShortType>()) ShortType();
    }

    unsigned int get_num_bits(bool is64Bit) final {
        return 16;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

    bool is_unsigned() final {
        return false;
    }

};