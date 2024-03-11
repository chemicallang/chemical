// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"
#include "ast/values/NotValue.h"
#include "ast/values/IncDecValue.h"
#include "ast/values/Negative.h"

void Parser::parseExpressionWith(ValueAndOperatorStack &stack, ValueAndOperatorStack &final) {
    while (position < tokens.size()) {
        auto valueOrAc = parseAccessChainOrValue();
        if(valueOrAc.has_value()) {
            final.putValue(valueOrAc->get());
            auto operation = consume_op_token();
            if(operation.has_value()) {
                auto precedence = operation.value();
                auto peak_precedence = stack.peakOperator();
                auto assocLeftToRight = is_assoc_left_to_right(operation.value());
                if (stack.isEmpty() || stack.peakChar() == '(') {
                    stack.putOperator(operation.value());
                } else if (!stack.has_operation_top() || precedence > peak_precedence) {
                    while (!stack.isEmpty() && stack.has_operation_top()) {
                        if (precedence > peak_precedence) {
                            final.putOperator(stack.popOperator());
                        } else if (precedence == peak_precedence) {
                            if(assocLeftToRight) {
                                final.putOperator(stack.popOperator());
                            } else {
                                // TODO cover RTL associativity
                            }
                        }
                    }
                    stack.putOperator(operation.value());
                } else if (precedence < peak_precedence) {
                    stack.putOperator(operation.value());
                } else if (precedence == peak_precedence) {
                    if(assocLeftToRight) {
                        final.putOperator(stack.popOperator());
                    } else {
                        // TODO cover RTL associativity
                    }
                    stack.putOperator(operation.value());
                } else {
                    error("No condition satisfied the ");
                }
            } else {
                break;
            }
        } else {
            if (consume_op('(')) {
                stack.putCharacter('(');
            } else if (consume_op(')')) {
                int found = false;
                while (!found) {
                    if (stack.has_operation_top()) {
                        final.putOperator(stack.popOperator());
                    } else if (stack.has_character_top()) {
                        if (stack.peakChar() == '(') found = true;
                        final.putCharacter(stack.popChar());
                    }
                }
            } else {
                break;
            }
        }
    }
    stack.putAllInto(final);
}

std::unique_ptr<Value> Parser::parseRemainingInterpretableExpression(std::unique_ptr<Value> firstValue) {
    auto op = consume_op_token();
    if (op.has_value()) {
        if (op.value() == Operation::PostfixIncrement) {
            return std::make_unique<IncDecValue>(std::move(firstValue), true);
        } else if(op.value() == Operation::PostfixDecrement) {
            return std::make_unique<IncDecValue>(std::move(firstValue), false);
        }

        ValueAndOperatorStack stack;
        stack.putOperator(op.value());
        ValueAndOperatorStack final;
        final.putValue(firstValue.release());
        parseExpressionWith(stack, final);
        return final.toExpression();
    } else {
        return firstValue;
    }
}

std::unique_ptr<Value> Parser::parseRemainingExpression(std::unique_ptr<Value> firstValue) {
    auto op = consume_op_token();
    if (op.has_value()) {
        if (op.value() == Operation::PostfixIncrement) {
            return std::make_unique<IncDecValue>(std::move(firstValue), true);
        } else if(op.value() == Operation::PostfixDecrement) {
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

    if(consume_op('-')) {
        auto value = parseExpression();
        if (value.has_value()) {
            return std::make_unique<NegativeValue>(std::move(value.value()));
        } else {
            error("expected a value after the negative '-' operator");
            return std::nullopt;
        }
    }

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