// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once

#include "ast/values/Expression.h"
#include "ast/values/CharValue.h"

class ValueAndOperatorStack {
public:

    enum class ItemType : uint8_t {
        Value,
        Char,
        Operation
    };

    union VOPItem {
        Value *value;
        char character;
        Operation operation;
    };

    struct VOPItemContainer {
        ItemType type;
        VOPItem item;
    };

    std::vector<VOPItemContainer> container;

    bool isEmpty() {
        return container.empty();
    }

    void putAllInto(ValueAndOperatorStack &other) {
        int i = container.size() - 1;
        while (i >= 0) {
            other.container.push_back(container[i]);
            i--;
        }
    }

    inline void putValue(Value *value) {
        container.push_back({ItemType::Value, value});
    }

    void putOperator(Operation operation) {
        container.push_back({ItemType::Operation, VOPItem{.operation = operation}});
    }

    void putCharacter(char character) {
        container.push_back({ItemType::Char, VOPItem{.character = character}});
    }

    std::optional<Operation> peakOperator() {
        if (has_operation_top()) {
            return (Operation) container.back().item.operation;
        } else {
            return std::nullopt;
        }
    }

    // check if on top of the stack is a operation, top means last element
    bool has_operation_top() {
        if (container.empty()) return false;
        return container.back().type == ItemType::Operation;
    }

    // check if on top of the stack is an operation, top means last element
    bool has_character_top() {
        if (container.empty()) return false;
        return container.back().type == ItemType::Char;
    }

    // check if on top of the stack is a value, top means last element
    bool has_value_top() {
        if (container.empty()) return false;
        return container.back().type == ItemType::Value;
    }

    Value *peakValue() {
        return has_value_top() ? container.back().item.value : nullptr;
    }

    std::optional<char> peakChar() {
        if (has_character_top()) {
            return container.back().item.character;
        } else {
            return std::nullopt;
        }
    }

    Operation popOperator() {
        auto op = peakOperator();
        container.pop_back();
        return op.value();
    }

    Value *popValue() {
        auto value = peakValue();
        if (value != nullptr) container.pop_back();
        return value;
    }

    char popChar() {
        auto c = peakChar();
        container.pop_back();
        return c.value();
    }

    std::unique_ptr<Expression> toExpression() {
        ValueAndOperatorStack stack;
        int i = 0;
        while(i < container.size()) {
            auto item = container[i];
            switch (item.type) {
                case ItemType::Value:
                    stack.putValue(item.item.value);
                    break;
                case ItemType::Char:
                    break;
                case ItemType::Operation:
                    auto second = stack.container.back();
                    stack.container.pop_back();
                    auto first = stack.container.back();
                    stack.container.pop_back();
                    stack.putValue(new Expression(std::unique_ptr<Value>(first.item.value),
                                                  std::unique_ptr<Value>(second.item.value), item.item.operation));
                    break;
            }
            i++;
        }
        return std::unique_ptr<Expression>((Expression *) stack.peakValue());
    }

};