// Copyright (c) Chemical Language Foundation 2025.

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
#include "utils/ValueAndOperatorStack.h"

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
            consumeNewLines();
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
                goto other_values;
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
            break;
        }
    }
    stack.putAllInto(final);
}

Value* Parser::parseRemainingExpression(ASTAllocator& allocator, Value* first_value, Token* start_tok) {

    auto operation = parseOperation();
    if(!operation.has_value()) {
        return first_value;
    }

    ValueAndOperatorStack stack;
    ValueAndOperatorStack final;

    stack.putOperator(operation.value());
    final.putValue(first_value);

    parseExpressionWith(allocator, stack, final);

    return final.toExpressionRaw(*this, allocator, is64Bit, loc_single(start_tok));

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

    lexNewLineChars();

    // lambda with no params
    if(consumeToken(TokenType::RParen)) {
        auto lamb = new (allocator.allocate<LambdaFunction>()) LambdaFunction(false, parent_node, loc_single(token));;
        parseLambdaAfterParamsList(allocator, lamb);
        return lamb;
    }

    const auto identifier = token;
    if(identifier->type == TokenType::Identifier || identifier->type == TokenType::SelfKw) {
        token++;
    } else {
        return nullptr;
    }

    if (consumeToken(TokenType::RParen)) {
        auto lamb = new (allocator.allocate<LambdaFunction>()) LambdaFunction(false, parent_node, loc_single(token));
        auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(allocate_view(allocator, identifier->value), nullptr, 0, nullptr, false, parent_node, loc_single(identifier));
        lamb->params.emplace_back(param);
        parseLambdaAfterParamsList(allocator, lamb);
        return lamb;
    } else if (consumeToken(TokenType::ColonSym)) {
        const auto typeLoc = parseTypeLoc(allocator);
        auto type = typeLoc.getType();
        if (type) {
        } else {
            unexpected_error("expected a type after ':' when lexing a lambda in parenthesized expression");
        }
        auto lamb = new (allocator.allocate<LambdaFunction>()) LambdaFunction(false, parent_node, loc_single(token));
        auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(allocate_view(allocator, identifier->value), typeLoc, 0, nullptr, false, parent_node, loc_single(identifier));
        lamb->params.emplace_back(param);
        if (consumeToken(TokenType::CommaSym)) {
            lamb->setIsVariadic(parseParameterList(allocator, lamb->params, true, false));
        }
        parseLambdaAfterComma(this, allocator,  lamb);
        return lamb;
    } else if (consumeToken(TokenType::CommaSym)) {
        auto lamb = new (allocator.allocate<LambdaFunction>()) LambdaFunction(false, parent_node, loc_single(token));
        auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(allocate_view(allocator, identifier->value), nullptr, 0, nullptr, false, parent_node, loc_single(identifier));
        lamb->params.emplace_back(param);
        lamb->setIsVariadic(parseParameterList(allocator, lamb->params, true, false));
        parseLambdaAfterComma(this, allocator, lamb);
        return lamb;
    }

    Value* first_value = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, identifier->value), loc_single(identifier), false);
    auto chain = new (allocator.allocate<AccessChain>()) AccessChain(loc_single(identifier));
    chain->values.emplace_back((ChainValue*) first_value);
    const auto structValue = parseAccessChainAfterId(allocator, chain->values, identifier->position);
    const auto value = structValue ? structValue : singlify_chain(chain);
    const auto finalValue = parseAfterValue(allocator, value, identifier);
    auto expr = parseRemainingExpression(allocator, finalValue, identifier);
    if(consumeToken(TokenType::RParen)) {
        const auto afterVl = parseAfterValue(allocator, expr, identifier);
        return parseRemainingExpression(allocator, afterVl, identifier);
    } else {
        error("expected ')' after the nested parenthesized expression");
        return expr;
    }

}

Value* Parser::parseParenExpression(ASTAllocator& allocator) {
    auto& tok = *token;

    if(tok.type == TokenType::LParen) {

        token++;

        auto expression = parseExpression(allocator, false, false);
        if(!expression) {
            error("expected a nested expression after '(' in the expression");
            return nullptr;
        }

        if (!consumeToken(TokenType::RParen)) {
            error("missing ')' in the expression");
            return nullptr;
        }

        return parseAfterValue(allocator, expression, &tok);

    } else {
        return nullptr;
    }
}

NotValue* Parser::parseNotValue(ASTAllocator& allocator) {
    auto& tok = *token;
    if (tok.type == TokenType::NotSym) {
        token++;
        auto parenExpression = parseParenExpression(allocator);
        if(parenExpression) {
            return new (allocator.allocate<NotValue>()) NotValue(parenExpression, loc_single(tok));
        } else {
            auto acValue = parsePostIncDec(allocator, parseAccessChainOrValueNoAfter(allocator, false), &tok);
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
        auto parenExpression = parseParenExpression(allocator);
        if(parenExpression) {
            return new (allocator.allocate<NegativeValue>()) NegativeValue(parenExpression, loc_single(tok));
        } else {
            auto acValue = parsePostIncDec(allocator, parseAccessChainOrValueNoAfter(allocator, false), &tok);
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

bool isKindChainValue(ValueKind kind) {
    switch(kind) {
        case ValueKind::AccessChain:
        case ValueKind::Identifier:
        case ValueKind::IndexOperator:
        case ValueKind::FunctionCall:
            return true;
        default:
            return false;
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

//    if(token->type == TokenType::LessThanSym && isKindChainValue(first_value->kind())) {
//        if(isGenericEndAhead()) {
//            consumeAny(); // consume the turbo fish
//            AccessChain* chain;
//            if (first_value->kind() == ValueKind::AccessChain) {
//                chain = first_value->as_access_chain_unsafe();
//            } else {
//                const auto new_chain = new(allocator.allocate<AccessChain>()) AccessChain(loc_single(start_tok));
//                new_chain->values.emplace_back((ChainValue*) first_value);
//                chain = new_chain;
//            };
//            std::vector<TypeLoc> genArgs;
//            parseGenericArgsListNoStart(genArgs, allocator);
//            if (token->type == TokenType::LParen) {
//                auto call = parseFunctionCall(allocator, chain->values);
//                call->generic_list = std::move(genArgs);
//                chain->values.emplace_back(call);
//            } else {
//                error("expected a '(' after the generic list in function call");
//            }
//            if (consumeToken(TokenType::DotSym)) {
//                auto value = parseAccessChainRecursive(allocator, chain->values, start_pos, false);
//                if (!value || value != chain) {
//                    error("expected a identifier after the dot . in the access chain");
//                    return nullptr;
//                }
//            }
//            return singlify_chain(chain);
//        }
//    }

    return parseRemainingExpression(allocator, first_value, &start_tok);

}

Value* Parser::parseExpressionOrArrayOrStruct(ASTAllocator& allocator, bool parseLambda) {
    switch(token->type) {
        case TokenType::LBracket:
            return parseArrayInit(allocator);
        case TokenType::LBrace:
            return (Value*) parseStructValue(allocator, nullptr, token->position);
        default:
            return parseExpression(allocator, true, parseLambda);
    }
}