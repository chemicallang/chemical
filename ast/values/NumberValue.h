// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/types/IntNType.h"
#include "IntNumValue.h"
#include "ast/types/IntType.h"
#include "TypeLinkedValue.h"

/**
 * @brief Class representing a number value integer / long
 *
 * suppose if user writes a function that returns long, parsing a int32 value would lose information
 * we could parse every value as a bigint value and return as appropriate type
 *
 * but it must link with the type it must return as, so this value is used, which automatically links
 * and returns as appropriate type automatically
 */
class NumberValue : public IntNumValue, public TypeLinkedValue {
public:

    int64_t value;
    std::unique_ptr<IntNType> linked_type = nullptr;
    CSTToken* token;

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    explicit NumberValue(int64_t value, CSTToken* token) : value(value), token(token) {}

    CSTToken* cst_token() override {
        return token;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *type) override;

    bool link(SymbolResolver &linker, ReturnStatement *returnStmt) override {
        return TypeLinkedValue::link(linker, returnStmt);
    }

    bool link(SymbolResolver &linker, StructValue *value, const std::string &name) override {
        return TypeLinkedValue::link(linker, value, name);
    }

    bool link(SymbolResolver &linker, FunctionCall *call, unsigned int index) override {
        return TypeLinkedValue::link(linker, call, index);
    }

    void relink_after_generic(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *expected_type) override {
        link(linker, value_ptr, expected_type);
    }

    bool link(SymbolResolver &linker, ArrayValue *value, unsigned int index) override {
        return TypeLinkedValue::link(linker, value, index);
    }

    bool computed() override {
        return true;
    }

    Value *scope_value(InterpretScope &scope) override;

    NumberValue* as_number_val() override {
        return this;
    }

    unsigned int get_num_bits() override;

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    ValueKind val_kind() override {
        return ValueKind::NumberValue;
    }

    NumberValue *copy() override {
        auto copy = new NumberValue(value, token);
        if(linked_type) {
            copy->linked_type = std::unique_ptr<IntNType>((IntNType *) linked_type->copy());
        }
        return copy;
    }

    bool is_unsigned() override;

    hybrid_ptr<BaseType> get_base_type() override {
        if(!linked_type) {
            return hybrid_ptr<BaseType> { (BaseType*) &IntType::instance, false };
        }
        return hybrid_ptr<BaseType> { linked_type.get(), false };
    }

    BaseType* known_type() override {
        if(!linked_type) {
            return (BaseType*) &IntType::instance;
        }
        return linked_type.get();
    }

    [[nodiscard]]
    std::unique_ptr<BaseType> create_type() override {
        if(linked_type) {
            return std::unique_ptr<BaseType>(linked_type->copy());
        } else {
            return std::unique_ptr<BaseType>(new IntType(nullptr));
        }
    }

    [[nodiscard]]
    ValueType value_type() const override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::IntN;
    }

};