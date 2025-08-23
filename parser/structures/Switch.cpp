// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/values/NullValue.h"
#include "ast/values/SwitchValue.h"
#include "ast/statements/SwitchStatement.h"

SwitchStatement* fix_switch(SwitchStatement* stmt, ASTAllocator& allocator) {
    if(!stmt->expression) {
        stmt->expression = new (allocator.allocate<NullValue>()) NullValue(nullptr, ZERO_LOC);
    }
    return stmt;
}

void Parser::parseSwitchStatementBlock(
        ASTAllocator& allocator,
        SwitchStatement* stmt,
        bool is_value,
        bool parse_value_node
) {

    if (consumeToken(TokenType::LParen)) {
        auto expr = parseExpression(allocator);
        if(expr) {
            stmt->expression = expr;
        } else {
            error("expected an expression tokens in switch statement");
            fix_switch(stmt, allocator);
            return;
        }
        if (!consumeToken(TokenType::RParen)) {
            error("expected ')' in switch statement");
            fix_switch(stmt, allocator);
            return;
        }
    } else {
        error("expect '(' after keyword 'switch' for the expression");
        fix_switch(stmt, allocator);
        return;
    }
    consumeNewLines();
    if (consumeToken(TokenType::LBrace)) {
        while(true) {
            consumeNewLines();
            bool has_single = false;
            int body_index = (int) stmt->scopes.size();
            do {
                if(has_single) {
                    consumeNewLines();
                }
                if(consumeToken(TokenType::DefaultKw)) {
                    has_single = true;
                    if(stmt->defScopeInd == -1) {
                        stmt->defScopeInd = body_index;
                    } else {
                        error("multiple default scopes detected");
                    }
                } else {
                    auto expr = parseExpression(allocator);
                    if(expr) {
                        has_single = true;
                        stmt->cases.emplace_back(expr, body_index);
                    } else {
                        break;
                    }
                }
            } while(consumeToken(TokenType::CommaSym));
            if(!has_single) {
                break;
            }
            stmt->scopes.emplace_back(parent_node, 0);
            auto& scope = stmt->scopes.back();
            if (consumeToken(TokenType::ColonSym)) {
                parseNestedLevelMultipleStatementsTokens(allocator, scope.nodes);
                continue;
            } else if (consumeToken(TokenType::LambdaSym)) {
                auto braceBlock = parseBraceBlockOrValueNode(allocator, "switch-case", is_value, parse_value_node);
                if(braceBlock.has_value()) {
                    scope = std::move(braceBlock.value());
                } else {
                    error("expected a brace block after the '=>' in the switch case");
                    return;
                }
            } else {
                error("expected ':' or '=>' after 'case' in switch statement");
                return;
            }
        }
        if(!consumeToken(TokenType::RBrace)) {
            error("expected '}' for ending the switch block");
            return;
        }
    } else {
        error("expected '{' after switch");
        return;
    }

}

SwitchStatement* Parser::parseSwitchStatementBlock(ASTAllocator& allocator, bool is_value, bool parse_value_node) {

    auto& tok = *token;

    if (tok.type != TokenType::SwitchKw) {
        return nullptr;
    }

    token++;

    auto stmt = new (allocator.allocate<SwitchStatement>()) SwitchStatement(nullptr, parent_node, loc_single(tok));

    parseSwitchStatementBlock(allocator, stmt, is_value, parse_value_node);

    return stmt;

}

SwitchValue* Parser::parseSwitchValue(ASTAllocator& allocator) {

    auto& tok = *token;

    if (tok.type != TokenType::SwitchKw) {
        return nullptr;
    }

    token++;

    const auto val = new (allocator.allocate<SwitchValue>()) SwitchValue(nullptr, parent_node, loc_single(tok));

    parseSwitchStatementBlock(allocator, &val->stmt, true, true);

    return val;

}