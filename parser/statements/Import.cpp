// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 07/03/2024.
//

#include "parser/Parser.h"
#include "ast/statements/Import.h"

bool Parser::parseImportStatementBool() {
    if (consume("import")) {
        auto value = consumeOfType<AbstractStringToken>(LexTokenType::String);
        if (value.has_value()) {
            nodes.emplace_back(std::make_unique<ImportStatement>(value.value()->value));
        } else {
            error("expected a value for the import statement");
        }
        return true;
    } else {
        return false;
    }
}