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

/**
 * Helper to parse symbol paths like a.b.c or a::b::c
 */
bool parseSymbolPath(BasicParser& parser, ASTAllocator& allocator, std::vector<chem::string_view>& parts) {
    do {
        auto id = parser.consumeIdentifierOrKeyword();
        if (id) {
            parts.push_back(parser.allocate_view(allocator, id->value));
        } else {
            break;
        }
    } while (consumeDotOrDCol(parser));
    return !parts.empty();
}

/**
 * Helper to parse a single item: "some.symbol as alias"
 */
bool parseImportItem(BasicParser& parser, ASTAllocator& allocator, ImportItem& item) {
    if (!parseSymbolPath(parser, allocator, item.parts)) return false;

    if (parser.consumeToken(TokenType::AsKw)) {
        auto alias = parser.consumeIdentifierOrKeyword();
        if (alias) {
            item.alias = parser.allocate_view(allocator, alias->value);
        } else {
            parser.unexpected_error("expected identifier after 'as' in import item");
            return false;
        }
    }
    return true;
}

/**
 * Parses the tail end of an import: version "x" subdir "y" branch "z" etc.
 */
void parseImportMetadata(BasicParser& parser, ASTAllocator& allocator, ImportStatement* stmt) {
    while (true) {
        if (parser.token->type != TokenType::Identifier) break;

        chem::string_view key = parser.token->value;
        parser.token++; // consume key

        auto val = parser.parseString(allocator);
        if (!val.has_value()) {
            parser.unexpected_error("expected string literal after metadata key");
            return;
        }

        if (key == "version") stmt->setVersion(val.value());
        else if (key == "subdir") stmt->setSubdir(val.value());
        else if (key == "branch") stmt->setBranch(val.value());
        else if (key == "commit") stmt->setCommit(val.value());
        else {
            parser.unexpected_error("unknown import metadata keyword");
        }
    }
}

ImportStatement* BasicParser::parseImportStmtAfterKw(ASTAllocator& allocator, bool error_out) {
    auto loc = loc_single(token);
    auto stmt = new (allocator.allocate<ImportStatement>()) ImportStatement(ImportStatementKind::NativeLib, "", parent_node, loc);

    // 1. Handle { symbol1, symbol2 }
    if (consumeToken(TokenType::LBrace)) {
        do {
            ImportItem item;
            if (parseImportItem(*this, allocator, item)) {
                stmt->addImportItem(std::move(item));
            }
            if (token->type == TokenType::RBrace) break;
        } while (consumeToken(TokenType::CommaSym));

        if (!consumeToken(TokenType::RBrace)) {
            unexpected_error("expected '}' after import list");
            return stmt;
        }

        if (!consumeToken(TokenType::FromKw)) {
            unexpected_error("expected 'from' after brace-enclosed import list");
            return stmt;
        }

        auto source = parseString(allocator);
        if (source.has_value()) {
            stmt->setSourcePath(source.value());
            stmt->setImportStmtKindDangerously(ImportStatementKind::LocalOrRemote);
        } else {
            // Handle 'from std' (identifier as source)
            std::vector<chem::string_view> pathParts;
            if (parseSymbolPath(*this, allocator, pathParts)) {
                // For simplicity, join parts back or store as string
                stmt->setSourcePath(pathParts[0]); // assuming single ID for native modules
            }
        }
    }
        // 2. Handle "path/string" as alias
    else if (auto str = parseString(allocator)) {
        stmt->setSourcePath(str.value());
        stmt->setImportStmtKindDangerously(ImportStatementKind::LocalOrRemote);
    }
        // 3. Handle identifier based: import a.b.c [from "path"] [as alias]
    else {
        std::vector<chem::string_view> primaryPath;
        if (parseSymbolPath(*this, allocator, primaryPath)) {
            // Check if this is "import symbol from source"
            if (consumeToken(TokenType::FromKw)) {
                ImportItem item;
                item.parts = std::move(primaryPath);
                stmt->addImportItem(std::move(item));

                auto source = parseString(allocator);
                if (source.has_value()) {
                    stmt->setSourcePath(source.value());
                    stmt->setImportStmtKindDangerously(ImportStatementKind::LocalOrRemote);
                }
                else {
                    std::vector<chem::string_view> srcParts;
                    if(parseSymbolPath(*this, allocator, srcParts)) stmt->setSourcePath(srcParts[0]);
                }
            } else {
                // It's just "import std" or "import std.sub"
                // We treat the first part as the source for the compatibility layer
                stmt->setSourcePath(primaryPath[0]);
                if (primaryPath.size() > 1) {
                    // remove the first item, before assigning
                    // import std.sub means import { sub } from std
                    primaryPath.erase(primaryPath.begin());
                    stmt->addImportItem(ImportItem{ .parts = std::move(primaryPath) });
                }
            }
        } else {
            if (error_out) unexpected_error("expected identifier or string in import");
            return nullptr;
        }
    }

    // 4. Global Alias: import ... as alias
    if (consumeToken(TokenType::AsKw)) {
        auto id = consumeIdentifierOrKeyword();
        if (id) stmt->setTopLevelAlias(allocate_view(allocator, id->value));
    }

    // 5. Remote Metadata (version, subdir, etc)
    parseImportMetadata(*this, allocator, stmt);

    // 6. Conditionals: if condition
    if (consumeToken(TokenType::IfKw)) {
        const auto iffy = parseIffyConditional(allocator);
        if (iffy) stmt->if_condition = iffy;
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