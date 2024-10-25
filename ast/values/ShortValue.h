// Copyright (c) Qinetik 2024.

#include "IntNumValue.h"
#include "ast/types/ShortType.h"

class ShortValue : public IntNumValue {
public:

    short value;
    SourceLocation location;

    explicit ShortValue(short value, SourceLocation location) : value(value), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::Short;
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &ShortType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &ShortType::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    ShortValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<ShortValue>()) ShortValue(value, location);
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<ShortType>()) ShortType(location);
    }

    unsigned int get_num_bits() final {
        return 16;
    }

    [[nodiscard]]
    int64_t get_num_value() const final {
        return value;
    }

    bool is_unsigned() final {
        return false;
    }

    [[nodiscard]] ValueType value_type() const final {
        return ValueType::Short;
    }

};