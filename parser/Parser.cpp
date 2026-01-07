// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Parser.h"
#include "ast/types/LinkedType.h"
#include "core/source/LocationManager.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/base/AnnotableNode.h"
#include "compiler/cbi/model/CompilerBinder.h"

Parser::Parser(
        unsigned int file_id,
        std::string_view file_path,
        Token* start_token,
        LocationManager& loc_man,
        AnnotationController& controller,
        ASTAllocator& global_allocator,
        ASTAllocator& mod_allocator,
        TypeBuilder& typeBuilder,
        bool is64Bit,
        CompilerBinder* binder
) : BasicParser(loc_man, file_id, start_token), stored_file_path(file_path),
    global_allocator(global_allocator), typeBuilder(typeBuilder), controller(controller),
    mod_allocator(mod_allocator), is64Bit(is64Bit), binder(binder)
{
    annotations.reserve(16);
}

std::string_view Parser::file_path() {
    return stored_file_path;
}

uint64_t BasicParser::loc(const Position& start, const Position& end) {
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

uint64_t BasicParser::loc_single(Position& pos, unsigned int length) {
    return loc_man.addLocation(file_id, pos.line, pos.character, pos.line, pos.character + length);
}

ASTNode* parseImportLevelStmt(Parser* parser, ASTAllocator& allocator) {
    while(true) {
        const auto type = parser->token->type;
        if(type == TokenType::NewLine) {
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
        auto stmt = parseTopLevelStatement(allocator, false);
        if(stmt) {
            nodes.emplace_back(stmt);
        } else if(!parseAnnotation(allocator)) {
            if (break_at_no_stmt || token->type == TokenType::EndOfFile) {
                break;
            } else {
                // skip the current token
                // error("skipped due to invalid syntax before it", token->position);
                token++;
                continue;
            }
        }
        consumeToken(TokenType::SemiColonSym);
    }
}

void Parser::parse(std::vector<ASTNode*>& nodes) {
    parseTopLevelMultipleStatements(mod_allocator, nodes);
}

chem::string_view BasicParser::get_file_path() {
    return chem::string_view(loc_man.getPathForFileId(file_id));
}

void BasicParser::consumeNewLines() {
    while(true) {
        if(token->type == TokenType::NewLine) {
            token++;
        } else {
            return;
        }
    }
}

void BasicParser::parseModuleFile(ASTAllocator& allocator, ModuleFileData& data) {
    auto& nodes = data.scope.body.nodes;
    consumeNewLines();
    const auto pkg_def = parseModuleDefinition(allocator, data);
    if(pkg_def) {
        consumeToken(TokenType::SemiColonSym);
    } else {
        return;
    }
    while (true) {
        consumeNewLines();
        switch(token->type) {
            case TokenType::Identifier: {
                const auto hash_fn = std::hash<chem::string_view>();
                switch(hash_fn(token->value)) {
                    case hash_fn("source"):
                        // handling source like this
                        if(!parseSourceStmt(allocator, data)) {
                            error("couldn't parse a source statement");
                            goto loop_break;
                        }
                        break;
                    case hash_fn("link"):
                        if(!parseLinkStmt(allocator, data)) {
                            error("couldn't parse a link statement");
                            goto loop_break;
                        }
                    break;
                }
                break;
            }
            case TokenType::ImportKw: {
                if(!parseSingleOrMultipleImportStatements(allocator, nodes)) {
                    goto loop_break;
                }
                break;
            }
            case TokenType::InterfaceKw: {
                token++;
                const auto id = consumeIdentifierOrKeyword();
                if(id) {
                    data.compiler_interfaces.emplace_back(allocate_view(allocator, id->value));
                    consumeToken(TokenType::SemiColonSym);
                }
                break;
            }
            case TokenType::NewLine:
                token++;
                continue;
            case TokenType::VarKw:
            case TokenType::ConstKw:
                // TODO handle these
            case TokenType::EndOfFile:
                goto loop_break;
            default: {
                // skip the current token
                error("skipped due to invalid syntax before it", token->position);
                token++;
                continue;
            }
        }
        consumeToken(TokenType::SemiColonSym);
    }
    loop_break:
        return;
}