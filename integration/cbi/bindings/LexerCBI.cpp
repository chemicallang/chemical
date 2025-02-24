// Copyright (c) Chemical Language Foundation 2025.

#include "LexerCBI.h"
#include "lexer/Lexer.h"

BatchAllocator* LexergetFileAllocator(Lexer* lexer) {
    return &lexer->file_allocator;
}

void LexersetUserLexer(Lexer* lexer, void* instance, void* subroutine) {
    if(lexer->user_lexer.instance != nullptr) {
        lexer->user_lexer_fns_stack.emplace_back(lexer->user_lexer);
    }
    lexer->other_mode = true;
    lexer->user_mode = true;
    lexer->user_lexer = UserLexerGetNextToken { instance, (UserLexerGetNextTokenFn) subroutine };
}

void LexerunsetUserLexer(Lexer* lexer) {
    if(lexer->user_lexer_fns_stack.empty()) {
        lexer->other_mode = false;
        lexer->user_mode = false;
        lexer->user_lexer.instance = nullptr;
        lexer->user_lexer.subroutine = nullptr;
    } else {
        const auto last = lexer->user_lexer_fns_stack.back();
        lexer->user_lexer_fns_stack.pop_back();
        lexer->user_lexer = last;
    }
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