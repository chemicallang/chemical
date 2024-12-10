// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/utils/Operation.h"
#include "parser/model/LexTokenType.h"
#include "CBIUtils.h"
#include "cst/base/CSTToken.h"

class Parser;

class SourceProvider;

extern "C" {

    std::size_t Lexertokens_size(Parser* lexer);

    CSTToken* Lexerput(Parser* lexer, chem::string* value, LexTokenType token_type, unsigned int lineNumber, unsigned int lineCharNumber);

}