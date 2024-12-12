// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Parser.h"
#include "ast/types/LinkedType.h"
#include "cst/LocationManager.h"
#include "CBI.h"
#include "ast/values/SizeOfValue.h"
#include "ast/base/AnnotableNode.h"

Parser::Parser(
        unsigned int file_id,
        std::string_view file_path,
        Token* start_token,
        LocationManager& loc_man,
        ASTAllocator& global_allocator,
        ASTAllocator& mod_allocator,
        GlobalInterpretScope& comptime_scope,
        bool is64Bit,
        CompilerBinder* binder
) : file_id(file_id), stored_file_path(file_path), token(start_token),
    loc_man(loc_man), global_allocator(global_allocator),
    mod_allocator(mod_allocator), is64Bit(is64Bit), binder(binder), comptime_scope(comptime_scope)
{

}

std::string_view Parser::file_path() {
    return stored_file_path;
}

uint64_t Parser::loc(const Position& start, const Position& end) {
    return loc_man.addLocation(file_id, start.line, start.character, end.line, end.character);
}

Position Parser::end_pos(Token* token) {
    return { token->position.line, (unsigned int) (token->position.character + token->value.size()) };
}

LocatedIdentifier Parser::loc_id(const std::string_view& value, const Position& pos) {
#ifdef LSP_BUILD
    return { std::string(value), loc_single(pos, value.size()) };
#else
    return { std::string(value) };
#endif
}

uint64_t Parser::loc_single(Position& pos, unsigned int length) {
    return loc_man.addLocation(file_id, pos.line, pos.character, pos.line, pos.character + length);
}

void Parser::parseTopLevelMultipleImportStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes) {
    while (true) {
        lexWhitespaceAndNewLines();
        auto importStmt = parseImportStatement(allocator);
        if(importStmt) {
            nodes.emplace_back((ASTNode*) importStmt);
        } else {
            auto comment = parseSingleLineComment(allocator);
            if(comment) {
                nodes.emplace_back((ASTNode*) comment);
            } else {
                auto multiline = parseMultiLineComment(allocator);
                if(multiline) {
                    nodes.emplace_back((ASTNode*) multiline);
                } else {
                    break;
                }
            }
        }
        lexWhitespaceToken();
        consumeToken(TokenType::SemiColonSym);
    }
}

void Parser::parseTopLevelMultipleStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes, bool break_at_no_stmt) {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while (true) {
        lexWhitespaceAndNewLines();
        auto stmt = parseTopLevelStatement(allocator);
        if(stmt) {
            nodes.emplace_back(stmt);
        } else if(!parseAnnotation(allocator)) {
            if (break_at_no_stmt || token->type == TokenType::EndOfFile) {
                break;
            } else {
                // skip the current token
                diagnostic(token->position, "skipped due to invalid syntax before it", DiagSeverity::Error);
                token++;
                continue;
            }
        }
        lexWhitespaceToken();
        consumeToken(TokenType::SemiColonSym);
    }
}

void Parser::parse(std::vector<ASTNode*>& nodes) {
    parseTopLevelMultipleImportStatements(mod_allocator, nodes);
    parseTopLevelMultipleStatements(mod_allocator, nodes);
}

Value* parseSizeOfValue(Parser* parser, ASTAllocator* allocator_ptr) {
    parser->readWhitespace();
    auto tok = parser->token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        parser->token++;
    } else {
        parser->error("expected '{' or '(' when parsing sizeof");
        return nullptr;
    }
    parser->readWhitespace();
    auto& allocator = *allocator_ptr;
    auto type = parser->parseType(allocator);
    if(type) {
        parser->readWhitespace();
        auto last = parser->token;
        auto value = new (allocator.allocate<SizeOfValue>()) SizeOfValue(type, parser->loc(tok, last));
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            parser->token++;
        } else {
            parser->error("expected '}' or '}' after the type when parsing sizeof");
        }
        return value;
    } else {
        parser->error("expected a type in #sizeof");
        return nullptr;
    }
}

Value* parseEvalValue(Parser* parser, ASTAllocator* allocator_ptr) {
    parser->readWhitespace();
    auto tok = parser->token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        parser->token++;
    } else {
        parser->error("expected '{' or '(' when parsing sizeof");
        return nullptr;
    }
    parser->readWhitespace();
    auto& allocator = *allocator_ptr;
    auto expr = parser->parseExpression(allocator);
    if(expr) {
        parser->readWhitespace();
        auto last = parser->token;
        auto evaluated = expr->evaluated_value((InterpretScope&) parser->comptime_scope);
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            parser->token++;
        } else {
            parser->error("expected '}' or '}' after the type when parsing sizeof");
        }
        return evaluated;
    } else {
        parser->error("expected a value in #eval");
        return nullptr;
    }
}