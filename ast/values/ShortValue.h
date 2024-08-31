// Copyright (c) Qinetik 2024.

#include "IntNumValue.h"
#include "ast/types/ShortType.h"

class ShortValue : public IntNumValue {
public:

    short value;
    CSTToken* token;

    explicit ShortValue(short value, CSTToken* token) : value(value), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::Short;
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { (BaseType*) &ShortType::instance, false };
    }

    BaseType* known_type() override {
        return (BaseType*) &ShortType::instance;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 2;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ShortValue *copy() override {
        return new ShortValue(value, token);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<ShortType>(nullptr);
    }

    unsigned int get_num_bits() override {
        return 16;
    }

    [[nodiscard]]
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