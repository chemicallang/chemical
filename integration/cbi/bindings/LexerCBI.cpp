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