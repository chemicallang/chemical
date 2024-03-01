// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"
#include "ast/values/NotValue.h"
#include "parser/utils/Operation.h"
#include "ast/values/IncDecValue.h"

std::optional<Operation> Parser::parseOperation() {
    auto value = get_op_token();
    if (value.has_value()) {
        switch (value.value()) {
            case '+':
                position++;
                return Operation::Addition;
            case '-':
                position++;
                return Operation::Subtraction;
            case '>':
                position++;
                return Operation::GreaterThan;
            case '<':
                position++;
                return Operation::LessThan;
            case '*':
                position++;
                return Operation::Multiplication;
            case '/':
                position++;
                return Operation::Division;
            case '%':
                position++;
                return Operation::Modulus;
            case '&':
                position++;
                return Operation::And;
            case '|':
                position++;
                return Operation::Or;
            case '^':
                position++;
                return Operation::Xor;
        }
    }
    auto strVal = consume_str_op();
    if (strVal.has_value()) {
        if (strVal.value() == "++") {
            return Operation::Increment;
        } else if (strVal.value() == "--") {
            return Operation::Decrement;
        } else if (strVal.value() == "<<") {
            return Operation::LeftShift;
        } else if (strVal.value() == ">>") {
            return Operation::RightShift;
        } else if (strVal.value() == "!=") {
            return Operation::NotEqual;
        } else if (strVal.value() == "==") {
            return Operation::Equal;
        } else if (strVal.value() == ">=") {
            return Operation::GreaterThanOrEqual;
        } else if (strVal.value() == "<=") {
            return Operation::LessThanOrEqual;
        }
    }
    return std::nullopt;
}

std::unique_ptr<Value> Parser::parseRemainingExpression(std::unique_ptr<Value> firstValue) {
    auto op = parseOperation();
    if (op.has_value()) {
        if (op.value() == Operation::Increment) {
            return std::make_unique<IncDecValue>(std::move(firstValue), true);
        } else if(op.value() == Operation::Decrement) {
            return std::make_unique<IncDecValue>(std::move(firstValue), false);
        }
        auto secondExpr = parseExpression();
        if (secondExpr.has_value()) {
            return std::make_unique<Expression>(std::move(firstValue), std::move(secondExpr.value()), op.value());
        } else {
            error("expected an expression after operation:" + to_string(op.value()));
            return firstValue;
        }
    } else {
        return firstValue;
    }
}


std::optional<std::unique_ptr<Value>> Parser::parseExpression() {

    if (consume_op('!')) {
        auto value = parseExpression();
        if (value.has_value()) {
            return std::make_unique<NotValue>(std::move(value.value()));
        } else {
            error("expected a value after the not '!' operator");
            return std::nullopt;
        }
    }

    if (consume_op('(')) {
        auto value = parseExpression();
        if (!value.has_value()) {
            error("expected a nested expression after parsing the '(' operator");
            return std::nullopt;
        }
        if (!consume_op(')')) {
            error("expected a ')' after the nested expression");
            return std::nullopt;
        }
        return parseRemainingExpression(std::move(value.value()));
    }

    auto firstValue = parseAccessChainOrValue();
    if (!firstValue.has_value()) {
        return std::nullopt;
    }

    return parseRemainingExpression(std::move(firstValue.value()));

}