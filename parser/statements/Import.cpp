// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 07/03/2024.
//

#include "parser/Parser.h"
#include "ast/statements/Import.h"

lex_ptr<ImportStatement> Parser::parseImportStatement() {
    if (consume("import")) {
        auto value = consumeOfType<AbstractStringToken>(LexTokenType::String);
        if (value != nullptr) {
            return std::make_unique<ImportStatement>(value->value);
        } else {
            error("expected a value for the import statement");
        }
    }
    return std::nullopt;
}

bool Parser::parseImportStatementBool() {
    return parse_return_bool([&] () -> lex_ptr<ImportStatement> {
        return parseImportStatement();
    });
}