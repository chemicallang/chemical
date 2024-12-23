// Copyright (c) Qinetik 2024.

#include "LexerCBI.h"
#include "lexer/Lexer.h"

BatchAllocator* LexergetFileAllocator(Lexer* lexer) {
    return &lexer->file_allocator;
}

void LexersetUserLexer(Lexer* lexer, void* instance, void* subroutine) {
    lexer->other_mode = true;
    lexer->user_mode = true;
    lexer->user_lexer = UserLexerGetNextToken { instance, (UserLexerGetNextTokenFn) subroutine };
}

void LexergetEmbeddedToken(Token* returning_token, Lexer* lexer) {
    const auto is_user_mode = lexer->other_mode && lexer->user_mode;
    if(is_user_mode) {
        lexer->other_mode = false;
        lexer->user_mode = false;
    }
    *returning_token = lexer->getNextToken();
    if(is_user_mode) {
        lexer->other_mode = true;
        lexer->user_mode = true;
    }
}