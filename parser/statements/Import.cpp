// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/statements/Import.h"

ImportStatement* Parser::parseImportStatement(ASTAllocator& allocator) {
    auto& kw_tok = *token;
    if (kw_tok.type != TokenType::ImportKw) {
        return nullptr;
    }
    auto stmt = new (allocator.allocate<ImportStatement>()) ImportStatement("", {}, parent_node, loc_single(kw_tok));
    token++;
    readWhitespace();
    auto str = parseStringValue(allocator);
    if (str) {
        stmt->filePath = str->get_the_string();
        if(lexWhitespaceToken() && consumeWSOfType(TokenType::AsKw)) {
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                stmt->as_identifier = id->value;
            } else {
                error("expected identifier after 'as' in import statement");
                return stmt;
            }
        }
    } else {
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            // TODO set the identifier in the statement
        } else {
            if (consumeToken(TokenType::LBrace)) {
                do {
                    lexWhitespaceAndNewLines();
                    auto identifier = consumeIdentifierOrKeyword();
                    if(!identifier) {
                        break;
                    }
                    lexWhitespaceToken();
                } while (consumeToken(TokenType::CommaSym));
                if (!consumeToken(TokenType::RBrace)) {
                    error("expected a closing bracket '}' after identifier list in import statement");
                }
            } else {
                error("expected a string path in import statement or identifier(s) after the 'import' keyword");
                return stmt;
            }
        }
        lexWhitespaceToken();
        if (consumeWSOfType(TokenType::FromKw)) {
            auto str2 = parseStringValue(allocator);
            if(str2) {
                // TODO handle this
            } else {
                error("expected path after 'from' in import statement");
                return stmt;
            }
        } else {
            error("expected keyword 'from' after the identifier");
            return stmt;
        }
    }
    return stmt;
}