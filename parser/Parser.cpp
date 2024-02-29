// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "Parser.h"

void Parser::parse() {
    eraseAllWhitespaceTokens();
    parseMultipleStatements();
}