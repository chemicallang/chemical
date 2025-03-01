// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Parser.h"
#include "ast/types/LinkedType.h"
#include "cst/LocationManager.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/base/AnnotableNode.h"

Parser::Parser(
        unsigned int file_id,
        std::string_view file_path,
        Token* start_token,
        LocationManager& loc_man,
        ASTAllocator& global_allocator,
        ASTAllocator& mod_allocator,
        bool is64Bit,
        CompilerBinder* binder
) : ASTDiagnoser(loc_man), file_id(file_id), stored_file_path(file_path), token(start_token),
    global_allocator(global_allocator),
    mod_allocator(mod_allocator), is64Bit(is64Bit), binder(binder)
{
    annotations.reserve(16);
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

LocatedIdentifier Parser::loc_id(BatchAllocator& allocator, const chem::string_view& value, const Position& pos) {
    const auto value_size = value.size();
    auto allocated = allocator.allocate_str(value.data(), value_size);
    return { chem::string_view(allocated, value_size) };
}

uint64_t Parser::loc_single(Position& pos, unsigned int length) {
    return loc_man.addLocation(file_id, pos.line, pos.character, pos.line, pos.character + length);
}

ASTNode* parseImportLevelStmt(Parser* parser, ASTAllocator& allocator) {
    // TODO comments should be attached to upcoming ast node
    while(true) {
        const auto type = parser->token->type;
        if(type == TokenType::SingleLineComment || type == TokenType::MultiLineComment || type == TokenType::NewLine) {
            parser->token++;
        } else {
            break;
        }
    }
    return (ASTNode*) parser->parseImportStatement(allocator);
}

void Parser::parseTopLevelMultipleImportStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes) {
    while (true) {
        consumeNewLines();
        const auto node = parseImportLevelStmt(this, allocator);
        if(node) {
            nodes.emplace_back((ASTNode*) node);
        } else {
            break;
        }
        consumeToken(TokenType::SemiColonSym);
    }
}

void Parser::parseTopLevelMultipleStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes, bool break_at_no_stmt) {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while (true) {
        consumeNewLines();
        auto stmt = parseTopLevelStatement(allocator);
        if(stmt) {
            nodes.emplace_back(stmt);
        } else if(!parseAnnotation(allocator)) {
            if (break_at_no_stmt || token->type == TokenType::EndOfFile) {
                break;
            } else {
                // skip the current token
                error("skipped due to invalid syntax before it", token->position);
                token++;
                continue;
            }
        }
        consumeToken(TokenType::SemiColonSym);
    }
}

void Parser::parse(std::vector<ASTNode*>& nodes) {
    parseTopLevelMultipleImportStatements(mod_allocator, nodes);
    parseTopLevelMultipleStatements(mod_allocator, nodes);
}