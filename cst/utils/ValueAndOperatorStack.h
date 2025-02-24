// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once

#include "parser/Parser.h"
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

    bool empty() const {
        return container.empty();
    }

    void clear() {
        container.clear();
    }

    inline bool isEmpty() {
        return empty();
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

    Operation peakOperator() {
        return (Operation) container.back().item.operation;
    }

    std::optional<Operation> safePeakOperator() {
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

    bool has_char_top(char value) {
        if (container.empty()) return false;
        auto& last = container.back();
        return last.type == ItemType::Char && last.item.character == value;
    }

    Value *peakValue() {
        return has_value_top() ? container.back().item.value : nullptr;
    }

    char peakChar() {
        return container.back().item.character;
    }

    std::optional<char> safePeakChar() {
        if (has_character_top()) {
            return container.back().item.character;
        } else {
            return std::nullopt;
        }
    }

    Operation popOperator() {
        auto op = peakOperator();
        container.pop_back();
        return op;
    }

    Value *popValue() {
        auto value = peakValue();
        if (value != nullptr) container.pop_back();
        return value;
    }

    char popChar() {
        auto c = peakChar();
        container.pop_back();
        return c;
    }

    Expression* toExpressionRaw(Parser& parser, ValueAndOperatorStack& stack, ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
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
                    if(stack.container.empty()) {
                        parser.ASTDiagnoser::error("couldn't find second value for making expression", second.item.value);
                    }
                    auto first = stack.container.back();
                    stack.container.pop_back();
                    stack.putValue(new (allocator.allocate<Expression>()) Expression(first.item.value,
                                                  second.item.value,
                                                          item.item.operation,
                                                          is64Bit,
                                                          location
                                                          )
                                                      );
                    break;
            }
            i++;
        }
        return (Expression *) stack.peakValue();
    }

    Expression* toExpressionRaw(Parser& parser, ASTAllocator& allocator, bool is64Bit, SourceLocation location) {
        ValueAndOperatorStack stack;
        return toExpressionRaw(parser, stack, allocator, is64Bit, location);
    }

};