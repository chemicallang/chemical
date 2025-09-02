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
        ShortType* shortType,
        SourceLocation location
    ) : IntNumValue(ValueKind::Short, shortType, location), value(value) {

    }

    inline ShortType* getType() const noexcept {
        return (ShortType*) IntNumValue::getType();
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    ShortValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<ShortValue>()) ShortValue(value, getType(), encoded_location());
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