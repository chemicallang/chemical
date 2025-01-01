// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/values/NotValue.h"
#include "ast/values/Negative.h"
#include "ast/values/CastedValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionParam.h"

void shunting_yard_on_operator(ValueAndOperatorStack& stack, ValueAndOperatorStack& final, Operation o1) {
    auto o1_precedence = to_precedence(o1);
    while (!stack.isEmpty() && stack.has_operation_top()) {
        auto o2_precedence = to_precedence(stack.peakOperator());
        if (o2_precedence > o1_precedence || (o1_precedence == o2_precedence && is_assoc_left_to_right(o1))) {
            final.putOperator(stack.popOperator());
        } else {
            break;
        }
    }
    stack.putOperator(o1);
}

/**
 * the implementation of shunting yard algorithm
 * https://en.wikipedia.org/wiki/Shunting_yard_algorithm
 */
void Parser::parseExpressionWith(ASTAllocator& allocator, ValueAndOperatorStack& stack, ValueAndOperatorStack& final) {

    // it's important to keep track of whether value or operation should be parsed first
    // Why: To differentiate between negative values and operations
    // in 3 - -4 <-- first minus symbol is subtraction operator, the second minus symbol creates negative 4 value
    // if we parse token by token, when we arrive -4, we can mistakenly parse second symbol as subtraction operator and put it onto stack
    // there's an opposite case where value is always parsed first and subtraction operator is consumed
    // inside the negative value, in a simple expression 3 - 4 doesn't mean 3 value and -4 value, it means
    // 3 value subtract 4 value
    // to solve this problem, we use a boolean value_first, which indicates value should be parsed first
    // otherwise operator is parsed first, because two operators or two values cannot appear consecutively
    // like 3 + + 4 or 3 value 4 value doesn't make it a valid expression

    bool value_first = true;
    while (token->type != TokenType::EndOfFile) {
        if(value_first) {
            auto valueOrAc = parseAccessChainOrValue(allocator);
            if(valueOrAc) {
                final.putValue(valueOrAc);
                value_first = false;
            } else {
                auto o1 = parseOperation();
                if(o1.has_value()) {
                    shunting_yard_on_operator(stack, final, o1.value());
                    value_first = true;
                } else {
                    goto other_values;
                }
            }
        } else {
            auto o1 = parseOperation();
            if(o1.has_value()) {
                shunting_yard_on_operator(stack, final, o1.value());
                value_first = true;
            } else {
                auto valueOrAc = parseAccessChainOrValue(allocator);
                if(valueOrAc) {
                    final.putValue(valueOrAc);
                    value_first = false;
                } else {
                    goto other_values;
                }
            }
        }
        continue;
        other_values:
        if (consumeToken(TokenType::LParen)) {
            stack.putCharacter('(');
        } else if (token->type == TokenType::RParen) {
            bool found = false;
            while (!stack.empty() && !found) {
                if (stack.has_operation_top()) {
                    final.putOperator(stack.popOperator());
                } else if (stack.has_character_top()) {
                    if (stack.peakChar() == '(') {
                        found = true;
                        stack.popChar();
                    } else {
                        final.putCharacter(stack.popChar());
                    }
                }
            }
            if (found) {
                token++;
            } else {
                break;
            }
        } else {
            if (token->type == TokenType::Whitespace) {
                token++;
            } else {
                break;
            }
        }
    }
    stack.putAllInto(final);
}

Value* Parser::parseRemainingExpression(ASTAllocator& allocator, Value* first_value, Token* start_tok) {

    lexWhitespaceToken();

    auto operation = parseOperation();
    if(!operation.has_value()) {
        return first_value;
    }

    readWhitespace();

    ValueAndOperatorStack stack;
    ValueAndOperatorStack final;

    stack.putOperator(operation.value());
    final.putValue(first_value);

    parseExpressionWith(allocator, stack, final);

    return final.toExpressionRaw(allocator, is64Bit, loc_single(start_tok));

}

bool parseLambdaAfterComma(Parser *lexer, ASTAllocator& allocator, LambdaFunction* func) {
    lexer->lexNewLineChars();
    if (!lexer->consumeToken(TokenType::RParen)) {
        lexer->error("expected ')' after the lambda parameter list in parenthesized expression");
        return false;
    }
    return lexer->parseLambdaAfterParamsList(allocator, func);
}

Value* Parser::parseLambdaOrExprAfterLParen(ASTAllocator& allocator) {

    lexWhitespaceToken();

    lexNewLineChars();

    // lambda with no params
    if(consumeToken(TokenType::RParen)) {
        auto lamb = new (allocator.allocate<LambdaFunction>()) LambdaFunction({}, {}, false, { nullptr, 0 }, parent_node, 0);;
        auto prev_func_type = current_func_type;
        current_func_type = lamb;
        parseLambdaAfterParamsList(allocator, lamb);
        current_func_type = prev_func_type;
        return lamb;
    }

    auto identifier = consumeIdentifierOrKeyword();
    if (!identifier) {
        return nullptr;
    }

    bool has_whitespace = lexWhitespaceToken();

    if (consumeToken(TokenType::RParen)) {
        auto lamb = new (allocator.allocate<LambdaFunction>()) LambdaFunction({}, {}, false, { nullptr, 0 }, parent_node, 0);
        auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(identifier->value.str(), nullptr, 0, nullptr, false, lamb, loc_single(identifier));
        lamb->params.emplace_back(param);
        auto prev_func_type = current_func_type;
        current_func_type = lamb;
        parseLambdaAfterParamsList(allocator, lamb);
        current_func_type = prev_func_type;
        return lamb;
    } else if (consumeToken(TokenType::ColonSym)) {
        lexWhitespaceToken();
        auto type = parseType(allocator);
        if (type) {
            lexWhitespaceToken();
        } else {
            error("expected a type after ':' when lexing a lambda in parenthesized expression");
        }
        auto lamb = new (allocator.allocate<LambdaFunction>()) LambdaFunction({}, {}, false, { nullptr, 0 }, parent_node, 0);
        auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(identifier->value.str(), type, 0, nullptr, false, lamb, loc_single(identifier));
        lamb->params.emplace_back(param);
        auto prev_func_type = current_func_type;
        current_func_type = lamb;
        if (consumeToken(TokenType::CommaSym)) {
            lamb->setIsVariadic(parseParameterList(allocator, lamb->params, true, false));
        }
        parseLambdaAfterComma(this, allocator,  lamb);
        current_func_type = prev_func_type;
        return lamb;
    } else if (consumeToken(TokenType::CommaSym)) {
        auto lamb = new (allocator.allocate<LambdaFunction>()) LambdaFunction({}, {}, false, { nullptr, 0 }, parent_node, 0);
        auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(identifier->value.str(), nullptr, 0, nullptr, false, lamb, loc_single(identifier));
        lamb->params.emplace_back(param);
        auto prev_func_type = current_func_type;
        current_func_type = lamb;
        lamb->setIsVariadic(parseParameterList(allocator, lamb->params, true, false));
        parseLambdaAfterComma(this, allocator, lamb);
        current_func_type = prev_func_type;
        return lamb;
    }

    if(has_whitespace) {
        auto first_value = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, identifier->value), loc_single(identifier), false);
        auto value = parseAfterValue(allocator, first_value, identifier);
        auto expr = parseRemainingExpression(allocator, value, identifier);
        if(consumeToken(TokenType::RParen)) {
            return parseRemainingExpression(allocator, expr, identifier);
        } else {
            auto second_expression = parseRemainingExpression(allocator, expr, identifier);
            if(!consumeToken(TokenType::RParen)) {
                error("expected ')' after the nested parenthesized expression");
            }
            return second_expression;
        }
    } else {
        auto chain = new (allocator.allocate<AccessChain>()) AccessChain(false, loc_single(identifier));
        auto value = parseAccessChainAfterId(allocator, chain, identifier->position);
        auto expr = parseRemainingExpression(allocator, value, identifier);
        if(!consumeToken(TokenType::RParen)) {
            error("expected a ')' after the access chain");
        }
        return expr;
    }
}

Value* Parser::parseParenExpression(ASTAllocator& allocator) {
    if(consumeToken(TokenType::LParen)) {

        auto expression = parseExpression(allocator, false, false);
        if(!expression) {
            error("expected a nested expression after '(' in the expression");
            return nullptr;
        }

        if (!consumeToken(TokenType::RParen)) {
            error("missing ')' in the expression");
            return nullptr;
        }

        return expression;

    } else {
        return nullptr;
    }
}

NotValue* Parser::parseNotValue(ASTAllocator& allocator) {
    auto& tok = *token;
    if (tok.type == TokenType::NotSym) {
        token++;
        readWhitespace();
        auto parenExpression = parseParenExpression(allocator);
        if(parenExpression) {
            return new (allocator.allocate<NotValue>()) NotValue(parenExpression, loc_single(tok));
        } else {
            auto acValue = parseAccessChainOrValue(allocator, false);
            if(acValue) {
                return new (allocator.allocate<NotValue>()) NotValue(acValue, loc_single(tok));
            } else {
                error("expected an expression after '!' not");
                return nullptr;
            }
        }
    } else {
        return nullptr;
    }
}

NegativeValue* Parser::parseNegativeValue(ASTAllocator& allocator) {
    auto& tok = *token;
    if (tok.type == TokenType::MinusSym) {
        token++;
        readWhitespace();
        auto parenExpression = parseParenExpression(allocator);
        if(parenExpression) {
            return new (allocator.allocate<NegativeValue>()) NegativeValue(parenExpression, loc_single(tok));
        } else {
            auto acValue = parseAccessChainOrValue(allocator, false);
            if(acValue) {
                return new (allocator.allocate<NegativeValue>()) NegativeValue(acValue, loc_single(tok));
            } else {
                error("expected an expression after '-' negative");
                return nullptr;
            }
        }
    } else {
        return nullptr;
    }
}

Value* Parser::parseExpression(ASTAllocator& allocator, bool parseStruct, bool parseLambda) {

    if (token->type == TokenType::LParen) {
        if(parseLambda) {
            token++;
            auto value = parseLambdaOrExprAfterLParen(allocator);
            if(value) {
                return value;
            } else {
                token--;
            }
        }
        const auto start_tok = token;
        auto parenExpression = parseParenExpression(allocator);
        if(parenExpression) {
            return parseRemainingExpression(allocator, parenExpression, start_tok);
        }
    }

    auto& start_tok = *token;
    auto& start_pos = start_tok.position;
    auto first_value = parseAccessChainOrValue(allocator, parseStruct);
    if(first_value) {
        if(parseStruct && first_value->val_kind() == ValueKind::StructValue) {
            return first_value;
        }
    } else {
        return nullptr;
    }

    lexWhitespaceToken();

    if (token->type == TokenType::LessThanSym && isGenericEndAhead()) {
        auto chain = new (allocator.allocate<AccessChain>()) AccessChain(false, loc_single(start_tok));
        std::vector<BaseType*> genArgs;
        parseGenericArgsList(genArgs, allocator);
        if(token->type == TokenType::LParen) {
            auto call = parseFunctionCall(allocator, chain);
            call->generic_list = std::move(genArgs);
        } else {
            error("expected a '(' after the generic list in function call");
        }
        if(consumeToken(TokenType::DotSym)) {
            auto value = parseAccessChainRecursive(allocator, chain, start_pos, false);
            if(!value || value != chain) {
                error("expected a identifier after the dot . in the access chain");
                return nullptr;
            }
        }
        return chain;
    }

    return parseRemainingExpression(allocator, first_value, &start_tok);

}