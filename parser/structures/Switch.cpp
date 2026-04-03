// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/values/NullValue.h"
#include "ast/values/SwitchValue.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/values/StructValue.h"

SwitchStatement* fix_switch(Parser* parser, ASTAllocator& allocator, SwitchStatement* stmt) {
    if(!stmt->expression) {
        stmt->expression = parser->getErroredValue(allocator);
    }
    return stmt;
}

void Parser::parseSwitchStatementBlock(
        ASTAllocator& allocator,
        SwitchStatement* stmt,
        bool is_value,
        bool parse_value_node
) {

    if (!consumeToken(TokenType::LParen)) {
        error("expect '(' after keyword 'switch' for the expression");
        fix_switch(this, allocator, stmt);
        return;
    }
    auto expr = parseExpression(allocator);
    if(expr == nullptr) {
        error("expected an expression tokens in switch statement");
        fix_switch(this, allocator, stmt);
        return;
    }
    stmt->expression = expr;
    if (!consumeToken(TokenType::RParen)) {
        error("expected ')' in switch statement");
        fix_switch(this, allocator, stmt);
        return;
    }
    consumeNewLines();
    if (!consumeToken(TokenType::LBrace)) {
        error("expected '{' after switch");
        return;
    }
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
                if (token->type == TokenType::Identifier && (token + 1)->type == TokenType::LBrace) {
                    const auto member_name = allocate_view(allocator, token->value);
                    auto namedType = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(member_name, stmt);
                    auto val = new (allocator.allocate<StructValue>()) StructValue(namedType, loc_single(token));
                    token++;
                    // parsing { name, name2 }
                    token++; // known lbrace
                    while (token->type == TokenType::Identifier) {
                        auto variable_name = allocate_view(allocator, token->value);
                        val->values.emplace(variable_name, StructMemberInitializer { variable_name, nullptr });
                        token++;
                        if (token->type == TokenType::CommaSym) token++;
                    }
                    if (token->type != TokenType::RBrace) {
                        error("expected r brace when named destructuring of variables in switch cases");
                    }
                    token++;
                    has_single = true;
                    stmt->cases.emplace_back(val, body_index);
                } else {
                    auto caseExpr = parseExpression(allocator);
                    if(caseExpr == nullptr) {
                        break;
                    }
                    has_single = true;
                    stmt->cases.emplace_back(caseExpr, body_index);
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
            if(!braceBlock.has_value()) {
                error("expected a brace block after the '=>' in the switch case");
                return;
            }
            scope = std::move(braceBlock.value());
        } else {
            error("expected ':' or '=>' after 'case' in switch statement");
            return;
        }
    }
    if(!consumeToken(TokenType::RBrace)) {
        error("expected '}' for ending the switch block");
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