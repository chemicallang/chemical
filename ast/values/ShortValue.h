// Copyright (c) Qinetik 2024.

#include "IntNumValue.h"

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

    unsigned int get_num_bits(bool is64Bit) override {
        return 16;
    }

    uint64_t get_num_value() override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::Short;
    }

};