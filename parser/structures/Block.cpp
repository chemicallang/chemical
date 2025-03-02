// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/Scope.h"
#include "ast/values/ValueNode.h"

void Parser::parseNestedLevelMultipleStatementsTokens(ASTAllocator& allocator, std::vector<ASTNode*>& nodes, bool is_value, bool parse_value_node) {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while(true) {
        consumeNewLines();
        auto stmt = parseNestedLevelStatementTokens(allocator, is_value, parse_value_node);
        if(stmt) {
            nodes.emplace_back(stmt);
        } else {
            if(!parseAnnotation(allocator)) {
                break;
            }
        }
        consumeToken(TokenType::SemiColonSym);
    }
}

std::optional<Scope> Parser::parseTopLevelBraceBlock(ASTAllocator& allocator, const std::string_view& forThing) {
    // whitespace and new lines
    consumeNewLines();

    // starting brace
    auto lb = consumeOfType(TokenType::LBrace);
    if (!lb) {
        return std::nullopt;
    }

    Scope scope(parent_node, loc_single(lb));

    // multiple statements
    parseTopLevelMultipleStatements(allocator, scope.nodes, true);

    // ending brace
    if (!consumeToken(TokenType::RBrace)) {
        error() << "expected a closing brace '}' for [" << forThing << "]";
    }

    return scope;
}

std::optional<Scope> Parser::parseBraceBlock(const std::string_view &forThing, ASTNode* new_parent_node, ASTAllocator& allocator) {
    auto prev_parent = parent_node;
    parent_node = new_parent_node;
    auto returnValue = parseNestedBraceBlock(forThing, allocator);
    parent_node = prev_parent;
    return returnValue;
}

std::optional<Scope> Parser::parseNestedBraceBlock(const std::string_view &forThing, ASTAllocator& allocator) {
    // whitespace and new lines
    consumeNewLines();

    // starting brace
    auto lb = consumeOfType(TokenType::LBrace);
    if (!lb) {
        return std::nullopt;
    }

    Scope scope(parent_node, loc_single(lb));

    // multiple statements
    parseNestedLevelMultipleStatementsTokens(allocator, scope.nodes);

    // ending brace
    if (!consumeToken(TokenType::RBrace)) {
        error() << "expected a closing brace '}' for [" << forThing << "]";
    }

    return scope;
}

std::optional<Scope> Parser::parseBraceBlockOrValueNode(ASTAllocator& allocator, const std::string_view& forThing, bool is_value, bool parse_value_node) {

    // whitespace and new lines
    consumeNewLines();

    // starting brace
    auto lb = consumeOfType(TokenType::LBrace);
    if (!lb) {
        if (parse_value_node) {
            auto valNode = parseValueNode(allocator);
            if (valNode) {
                consumeNewLines();
                if (consumeToken(TokenType::SemiColonSym)) {
                    consumeNewLines();
                }
                return Scope{{valNode}, parent_node, valNode->encoded_location()};
            } else {
                return std::nullopt;
            }
        }
        auto nested_stmt = parseNestedLevelStatementTokens(allocator, is_value, parse_value_node);
        if (nested_stmt) {
            consumeNewLines();
            if (consumeToken(TokenType::SemiColonSym)) {
                consumeNewLines();
            }
            return Scope{{nested_stmt}, parent_node, nested_stmt->encoded_location()};
        }
        return std::nullopt;
    }

    Scope scope(parent_node, 0);

    // multiple statements
    parseNestedLevelMultipleStatementsTokens(allocator, scope.nodes, false, parse_value_node);

    // ending brace
    auto rb = consumeOfType(TokenType::RBrace);
    if (!rb) {
        error() << "expected a closing brace '}' for [" << forThing << "]";
        return scope;
    }

    scope.set_encoded_location(loc(lb->position, rb->position));

    return scope;

}