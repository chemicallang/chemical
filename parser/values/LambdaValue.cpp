// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/values/LambdaFunction.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/statements/Return.h"

bool Parser::parseLambdaAfterParamsList(ASTAllocator& allocator, LambdaFunction* lambda) {


    if (!consumeToken(TokenType::LambdaSym)) {
        error("expected '=>' for a lambda");
        return false;
    }


    auto braceBlock = parseBraceBlock("lambda", parent_node, allocator);
    if(braceBlock.has_value()) {
        lambda->scope = std::move(braceBlock.value());
    } else {
        auto expr = parseExpression(allocator);
        if(expr) {
            auto returnStmt = new (allocator.allocate<ReturnStatement>()) ReturnStatement(expr, lambda, &lambda->scope, ZERO_LOC);
            lambda->scope.nodes.emplace_back(returnStmt);
        } else {
            error("expected lambda body");
            return false;
        }
    }

    return true;
}

LambdaFunction* Parser::parseLambdaValue(ASTAllocator& allocator) {
    if (consumeToken(TokenType::LBracket)) {

        auto lambda = new (allocator.allocate<LambdaFunction>()) LambdaFunction(false, parent_node, 0);

        auto prev_func_type = current_func_type;
        current_func_type = lambda;

        lambda->setIsCapturing(true);

        // the capture list
        unsigned int index = 0;
        do {
            consumeNewLines();
            bool lexed_amp = consumeOfType(TokenType::AmpersandSym);
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                auto variable = new (allocator.allocate<CapturedVariable>()) CapturedVariable(allocate_view(allocator, id->value), index, lexed_amp, parent_node, loc_single(id));
                variable->lambda = lambda;
                lambda->captureList.emplace_back(variable);
            } else {
                if(lexed_amp) {
                    error("expected identifier after '&'");
                    return lambda;
                }
            }
            index++;
        } while (consumeToken(TokenType::CommaSym));

        if (!consumeToken(TokenType::RBracket)) {
            error("expected ']' after lambda function capture list");
            return lambda;
        }

        if (!consumeToken(TokenType::LParen)) {
            error("expected '(' for lambda parameter list");
            return lambda;
        }

        auto isVariadic = parseParameterList(allocator, lambda->params, true, false);
        lambda->setIsVariadic(isVariadic);

        lexNewLineChars();

        if (!consumeToken(TokenType::RParen)) {
            error("expected ')' after the lambda parameter list");
            return lambda;
        }

        if(!parseLambdaAfterParamsList(allocator, lambda)) {
            return lambda;
        }

        current_func_type = prev_func_type;

        return lambda;
    } else {
        return nullptr;
    }
}