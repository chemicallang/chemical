// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/values/LambdaFunction.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/statements/Return.h"

bool Parser::parseLambdaAfterParamsList(ASTAllocator& allocator, LambdaFunction* lambda) {


    if (!consumeToken(TokenType::LambdaSym)) {
        unexpected_error("expected '=>' for a lambda");
        return false;
    }


    auto braceBlock = parseBraceBlock("lambda", parent_node, allocator);
    if(braceBlock.has_value()) {
        lambda->scope = std::move(braceBlock.value());
    } else {
        auto expr = parseExpression(allocator);
        if(expr) {
            auto returnStmt = new (allocator.allocate<ReturnStatement>()) ReturnStatement(expr, &lambda->scope, ZERO_LOC);
            lambda->scope.nodes.emplace_back(returnStmt);
        } else {
            unexpected_error("expected lambda body");
            return false;
        }
    }

    return true;
}

LambdaFunction* Parser::parseLambdaValue(ASTAllocator& allocator) {

    auto lambda = new (allocator.allocate<LambdaFunction>()) LambdaFunction(false, parent_node, loc_single(token));

    if(consumeToken(TokenType::PipeSym)) {

        lambda->setIsCapturing(true);

        // the capture list
        unsigned int index = 0;
        do {
            consumeNewLines();
            bool lexed_amp = consumeToken(TokenType::AmpersandSym);
            bool lexed_mut = lexed_amp && consumeToken(TokenType::MutKw);
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                auto variable = new (allocator.allocate<CapturedVariable>()) CapturedVariable(allocate_view(allocator, id->value), index, lexed_amp, lexed_mut, parent_node, loc_single(id));
#ifdef LSP_BUILD
                id->linked = variable;
#endif
                lambda->captureList.emplace_back(variable);
            } else {
                if(lexed_amp) {
                    unexpected_error("expected identifier after '&'");
                    return lambda;
                }
            }
            index++;
        } while (consumeToken(TokenType::CommaSym));

        if (!consumeToken(TokenType::PipeSym)) {
            unexpected_error("expected '|' after lambda function capture list");
            return lambda;
        }

    } else if(consumeToken(TokenType::LogicalOrSym)) {
        lambda->setIsCapturing(true);
    } else {
        return nullptr;
    }

    if (!consumeToken(TokenType::LParen)) {
        unexpected_error("expected '(' for lambda parameter list");
        return lambda;
    }

    auto isVariadic = parseParameterList(allocator, lambda->params, true, false);
    lambda->setIsVariadic(isVariadic);

    lexNewLineChars();

    if (!consumeToken(TokenType::RParen)) {
        unexpected_error("expected ')' after the lambda parameter list");
        return lambda;
    }

    if(!parseLambdaAfterParamsList(allocator, lambda)) {
        return lambda;
    }

    return lambda;

}