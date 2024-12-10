// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/statements/SwitchStatement.h"

SwitchStatement* Parser::parseSwitchStatementBlock(ASTAllocator& allocator, bool is_value, bool parse_value_node) {
    if (consumeWSOfType(TokenType::SwitchKw)) {

        auto stmt = new (allocator.allocate<SwitchStatement>()) SwitchStatement(nullptr, parent_node, is_value, 0);

        if (consumeToken(TokenType::LParen)) {
            auto expr = parseExpression(allocator);
            if(expr) {
                stmt->expression = expr;
            } else {
                error("expected an expression tokens in switch statement");
                return stmt;
            }
            if (!consumeToken(TokenType::RParen)) {
                error("expected ')' in switch statement");
                return stmt;
            }
        } else {
            error("expect '(' after keyword 'switch' for the expression");
            return stmt;
        }
        lexWhitespaceAndNewLines();
        if (consumeToken(TokenType::LBrace)) {
            while(true) {
                lexWhitespaceAndNewLines();
                bool has_single = false;
                int body_index = (int) stmt->scopes.size();
                do {
                    if(has_single) {
                        lexWhitespaceToken();
                    }
                    if(consumeWSOfType(TokenType::DefaultKw)) {
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
                    lexWhitespaceToken();
                } while(consumeToken(TokenType::CommaSym));
                if(!has_single) {
                    break;
                }
                lexWhitespaceToken();
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
                        return stmt;
                    }
                } else {
                    error("expected ':' or '=>' after 'case' in switch statement");
                    return stmt;
                }
            }
            if(!consumeToken(TokenType::RBrace)) {
                error("expected '}' for ending the switch block");
                return stmt;
            }
        } else {
            error("expected '{' after switch");
            return stmt;
        }
        return stmt;
    }
    return nullptr;
}