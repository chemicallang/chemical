// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"

/**
 * Parses: index_with "<indexer>", "<indexer>" ;
 *
 * Each value names a compiler plugin that indexes this module's public surface.
 * The statement is repeatable; every parsed name is appended to
 * `data.indexers`. A non-string value is diagnosed at its source location.
 */
bool BasicParser::parseIndexWithStmt(ASTAllocator& allocator, ModuleFileData& data) {
    // The module-file dispatch sees the `index_with` identifier itself (it is
    // not a dedicated token). Consume it if present.
    if (token->type == TokenType::Identifier && token->value == "index_with") {
        token++;
    } else {
        return false;
    }

    while (true) {
        if (token->type == TokenType::EndOfFile) {
            error("unexpected end of file in 'index_with' statement, expected a string");
            return false;
        }

        auto indexer = parseString(allocator);
        if (!indexer.has_value()) {
            error("expected a string value for 'index_with'");
            return false;
        }
        data.indexers.emplace_back(indexer.value());

        if (!consumeToken(TokenType::CommaSym)) {
            break;
        }
        // allow a comma-separated list to span lines
        consumeNewLines();
    }

    return true;
}
