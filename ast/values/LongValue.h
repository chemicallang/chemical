// Copyright (c) Qinetik 2024.

#include "IntNumValue.h"

class LongValue : public IntNumValue {
public:

    long value;

    LongValue(long value) : value(value) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    Value *copy() override {
        return new LongValue(value);
    }

    std::string representation() const override {
        return std::to_string(value);
    }

    unsigned int get_num_bits(bool is64Bit) override {
        if(is64Bit) {
            return 64;
        } else {
            return 32;
        }
    }

    uint64_t get_num_value() override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::Long;
    }

};