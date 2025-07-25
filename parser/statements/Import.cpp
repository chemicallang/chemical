// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/statements/Import.h"

inline bool consumeDotOrDCol(BasicParser& parser) {
    switch(parser.token->type) {
        case TokenType::DoubleColonSym:
        case TokenType::DotSym:
            parser.token++;
            return true;
        default:
            return false;
    }
}

bool parseImportFromPart(BasicParser& parser, ASTAllocator& allocator, ImportStatement* stmt) {
    if (parser.consumeToken(TokenType::FromKw)) {
        auto str2 = parser.parseString(allocator);
        if(str2.has_value()) {
            stmt->filePath = str2.value();
            return true;
        } else {
            parser.unexpected_error("expected path after 'from' in import statement");
            return false;
        }
    } else {
        return true;
    }
}

ImportStatement* BasicParser::parseImportStmtAfterKw(ASTAllocator& allocator, bool error_out) {
    auto stmt = new (allocator.allocate<ImportStatement>()) ImportStatement("", parent_node, loc_single(token));
    auto str = parseString(allocator);
    if (str.has_value()) {
        stmt->filePath = str.value();
        if(consumeToken(TokenType::AsKw)) {
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                stmt->as_identifier = allocate_view(allocator, id->value);
            } else {
                unexpected_error("expected identifier after 'as' in import statement");
                return stmt;
            }
        }
    } else {
        do {
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                stmt->identifier.emplace_back(allocate_view(allocator, id->value));
            } else {
                break;
            }
        } while (consumeDotOrDCol(*this));
        if(stmt->identifier.empty()) {
            if(error_out) {
                unexpected_error("expected a single identifier or a string path in import statement");
                return stmt;
            } else {
                return nullptr;
            }
        }
        if(!parseImportFromPart(*this, allocator, stmt)) {
            return stmt;
        }
        if(consumeToken(TokenType::AsKw)) {
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                stmt->as_identifier = allocate_view(allocator, id->value);
            } else {
                unexpected_error("expected identifier after 'as' in import statement");
                return stmt;
            }
        }
    }
    if(consumeToken(TokenType::IfKw)) {
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            stmt->if_condition = allocate_view(allocator, id->value);
        } else {
            unexpected_error("expected if condition identifier in import statement");
        }
    }
    return stmt;
}

ImportStatement* BasicParser::parseImportStatement(ASTAllocator& allocator) {
    auto& kw_tok = *token;
    if (kw_tok.type != TokenType::ImportKw) {
        return nullptr;
    }
    token++;
    return parseImportStmtAfterKw(allocator, false);
}

bool BasicParser::parseSingleOrMultipleImportStatements(ASTAllocator& allocator, std::vector<ASTNode*>& nodes) {
    auto& kw_tok = *token;
    if (kw_tok.type != TokenType::ImportKw) {
        return false;
    }
    token++;
    if(token->type == TokenType::LParen) {
        token++;

        const auto start = nodes.size();

        // parse imports optionally
        while(true) {
            consumeNewLines();
            const auto single = parseImportStmtAfterKw(allocator, false);
            if(single) {
                nodes.emplace_back(single);
            } else {
                break;
            }
        }

        if(token->type == TokenType::RParen) {
            token++;
        }

        if(start == nodes.size()) {
            error("not a single import inside the import list found");
            return true;
        }

        if(token->type == TokenType::FromKw) {

            const auto first = nodes[start]->as_import_stmt_unsafe();
            if(!first->filePath.empty()) {
                error("import statement inside the import list contains a path even though list has a path");
                return true;
            }

            if(!parseImportFromPart(*this, allocator, first)) {
                return true;
            }

            // give all imports the path
            unsigned i = start;
            while(i < nodes.size()) {
                const auto imp = nodes[i]->as_import_stmt_unsafe();
                if(imp->filePath.empty()) {
                   imp->filePath = first->filePath;
                } else {
                    error("import statement inside the import list already has a path");
                }
                i++;
            }

        } else {
            // check all imports have paths
            unsigned i = start;
            while(i < nodes.size()) {
                const auto imp = nodes[i]->as_import_stmt_unsafe();
                if(imp->filePath.empty()) {
                    error("import statement inside the import list is missing a path");
                }
                i++;
            }
        }

        return true;

    } else {
        const auto single = parseImportStmtAfterKw(allocator, false);
        if(single) {
            nodes.emplace_back(single);
            return true;
        } else {
            unexpected_error("expected an import inside the import list");
            return false;
        }
    }
}