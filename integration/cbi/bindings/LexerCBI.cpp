// Copyright (c) Qinetik 2024.

#include "LexerCBI.h"
#include "parser/Parser.h"

std::size_t Lexertokens_size(Parser* lexer)  {
    return lexer->tokens_size();
}

CSTToken* Lexerput(Parser* lexer, chem::string* value, LexTokenType token_type, unsigned int lineNumber, unsigned int lineCharNumber) {
    lexer->emplace(token_type, { lineNumber, lineCharNumber }, value->to_view());
    return lexer->unit.tokens.back();
}