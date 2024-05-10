// Copyright (c) Qinetik 2024.

#include "IntNumValue.h"
#include "ast/types/ShortType.h"

class ShortValue : public IntNumValue {
public:

    short value;

    ShortValue(short value) : value(value) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    Value *copy() override {
        return new ShortValue(value);
    }

    std::string representation() const override {
        return std::to_string(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<ShortType>();
    }

    unsigned int get_num_bits() override {
        return 16;
    }

    int64_t get_num_value() const override {
        return value;
    }

    bool is_unsigned() override {
        return false;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::Short;
    }

};