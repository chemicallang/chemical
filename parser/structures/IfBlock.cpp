// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/If.h"

std::optional<std::pair<Value*, Scope>> Parser::parseIfExprAndBlock(ASTAllocator& allocator, bool is_value, bool lex_value_node, bool top_level) {

    auto lp = consumeOfType(TokenType::LParen);
    if (!lp) {
        error("expected a starting parenthesis ( when lexing a if block");
        return std::nullopt;
    }

    auto expr = parseExpression(allocator);
    if(!expr) {
        error("expected a conditional expression when lexing a if block");
        return std::nullopt;
    }

    consumeNewLines();

    if (!consumeToken(TokenType::RParen)) {
        error("expected a ending parenthesis ) when lexing a if block");
        return std::pair { expr, Scope { parent_node, loc_single(lp) } };
    }

    if(top_level) {
        auto scope = parseTopLevelBraceBlock(allocator, "else");
        if(scope.has_value()) {
            return std::pair { expr, std::move(scope.value()) };
        } else {
            error("expected a brace block after the else while lexing an if statement");
            return std::pair { expr, Scope { parent_node, loc_single(lp) } };
        }
    } else {
        auto blk = parseBraceBlockOrValueNode(allocator, "if", is_value, lex_value_node);
        if(blk.has_value()) {
            return std::pair { expr, std::move(blk.value()) };
        } else {
            error("expected a brace block when lexing a brace block");
            return std::pair { expr, Scope { parent_node, loc_single(lp) } };
        }
    }

}

IfStatement* Parser::parseIfStatement(ASTAllocator& allocator, bool is_value, bool parse_value_node, bool top_level) {

    auto& first = *token;
    if(first.type != TokenType::IfKw) {
        return nullptr;
    }

    token++;

    auto statement = new (allocator.allocate<IfStatement>()) IfStatement(nullptr, parent_node, is_value, loc_single(first));

    auto exprBlock = parseIfExprAndBlock(allocator, is_value, parse_value_node, top_level);
    if(exprBlock.has_value()) {
        auto& exprBlockValue = exprBlock.value();
        statement->condition = exprBlockValue.first;
        statement->ifBody = std::move(exprBlockValue.second);
    } else {
        return statement;
    }

    // lex whitespace
    consumeNewLines();

    // keep lexing else if blocks until last else appears
    while (consumeWSOfType(TokenType::ElseKw) != nullptr) {
        consumeNewLines();
        if(consumeWSOfType(TokenType::IfKw)) {
            auto exprBlock2 = parseIfExprAndBlock(allocator, is_value, parse_value_node, top_level);
            if(exprBlock2.has_value()) {
                statement->elseIfs.emplace_back(std::move(exprBlock2.value()));
            } else {
                return statement;
            }
        } else {
            if(top_level) {
                // we pass the module allocator
                // because public nodes inside if stmt still use global allocator and
                // non public nodes will use this module allocator, because we explicitly
                // remove these nodes when the module has generated code
                auto block = parseTopLevelBraceBlock(mod_allocator, "else");
                if(block.has_value()) {
                    statement->elseBody = std::move(block.value());
                } else {
                    error("expected a brace block after the else while parsing an if statement");
                    return statement;
                }
            } else {
                auto block = parseBraceBlockOrValueNode(allocator, "else", is_value, parse_value_node);
                if(block.has_value()) {
                    statement->elseBody = std::move(block.value());
                } else {
                    error("expected a brace block after the else while lexing an if statement");
                    return statement;
                }
            }
            return statement;
        }
    }

    return statement;

}