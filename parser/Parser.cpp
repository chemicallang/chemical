// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "Parser.h"

/**
 * This parses the given tokens from the lexer
 * converts it into a ast and then returns it
 * @param tokens
 * @return
 */
void Parser::parse() {
    eraseAllWhitespaceTokens();
    parseVariableInitStatement();
}