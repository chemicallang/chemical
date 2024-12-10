// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/statements/Import.h"

bool Parser::lexImportIdentifierList() {
    if (lexOperatorToken(TokenType::LBrace)) {
        do {
            lexWhitespaceAndNewLines();
            if (!lexIdentifierToken()) {
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(TokenType::CommaSym));
        if (!lexOperatorToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' after identifier list in import statement");
        }
        return true;
    } else {
        return false;
    }
}

ImportStatement* Parser::parseImportStatement(ASTAllocator& allocator) {
    if (!lexWSKeywordToken(TokenType::ImportKw)) {
        return nullptr;
    }
    auto stmt = new (allocator.allocate<ImportStatement>()) ImportStatement("", {}, parent_node, 0);
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
        if (lexIdentifierToken() || lexImportIdentifierList()) {
            lexWhitespaceToken();
            if (lexWSKeywordToken(TokenType::FromKw)) {
                if (!lexStringToken()) {
                    error("expected path after 'from' in import statement");
                    return stmt;
                }
            } else {
                error("expected keyword 'from' after the identifier");
                return stmt;
            }
        } else {
            error("expected a string path in import statement or identifier(s) after the 'import' keyword");
            return stmt;
        }
    }
    return stmt;
}

bool Parser::lexImportStatement() {
    if (!lexWSKeywordToken(TokenType::ImportKw)) {
        return false;
    }
    unsigned int start = tokens_size() - 1;
    if (lexStringToken()) {
        if(lexWhitespaceToken() && lexWSKeywordToken(TokenType::AsKw)) {
            if(!lexIdentifierToken()) {
                error("expected identifier after 'as' in import statement");
                return true;
            }
        }
        if(lexWhitespaceToken() && lexWSKeywordToken(TokenType::IfKw)) {
            if(!lexIdentifierToken()) {
                error("Expected identifier after 'if' in import statement");
                return true;
            }
        }
        compound_from(start, LexTokenType::CompImport);
        return true;
    } else {
        if (lexIdentifierToken() || lexImportIdentifierList()) {
            lexWhitespaceToken();
            if (lexWSKeywordToken(TokenType::FromKw)) {
                if (!lexStringToken()) {
                    error("expected path after 'from' in import statement");
                    return true;
                }
            } else {
                error("expected keyword 'from' after the identifier");
                return true;
            }
        } else {
            error("expected a string path in import statement or identifier(s) after the 'import' keyword");
            return true;
        }
        compound_from(start, LexTokenType::CompImport);
    }
    return true;
}