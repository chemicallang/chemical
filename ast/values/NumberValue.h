// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/types/IntNType.h"
#include "IntNumValue.h"
#include "ast/types/IntType.h"

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

    int64_t value;
    std::unique_ptr<IntNType> linked_type = nullptr;

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    NumberValue(int64_t value) : value(value) {}

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, VarInitStatement *stmnt) override;

    void link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs) override;

    void link(SymbolResolver &linker, ReturnStatement *returnStmt) override;

    void link(SymbolResolver &linker, FunctionCall *call, unsigned int index) override;

    void link(SymbolResolver &linker, StructValue *value, const std::string &name) override;

    bool computed() override {
        return true;
    }

    Value *scope_value(InterpretScope &scope) override;

    NumberValue* as_number_val() override {
        return this;
    }

    unsigned int get_num_bits() override;

    int64_t get_num_value() const override {
        return value;
    }

    Value *copy() override {
        auto copy = new NumberValue(value);
        copy->linked_type = std::unique_ptr<IntNType>((IntNType*) linked_type->copy());
        return copy;
    }

    bool is_unsigned() override;

    hybrid_ptr<BaseType> get_base_type() override {
        if(linked_type) {
            return hybrid_ptr<BaseType> {linked_type->copy(), true};
        } else {
            return hybrid_ptr<BaseType> { new IntType(), true };
        }
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        if(linked_type) {
            return std::unique_ptr<BaseType>(linked_type->copy());
        } else {
            return std::unique_ptr<BaseType>(new IntType());
        }
    }

    int as_int() override {
        return value;
    }

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::IntN;
    }

};