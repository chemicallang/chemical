// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class BatchAllocator;

class Lexer;

struct Token;

extern "C" {

    BatchAllocator* LexergetFileAllocator(Lexer* lexer);

    void LexersetUserLexer(Lexer* lexer, void* instance, void* subroutine);

    void LexerunsetUserLexer(Lexer* lexer);

    void LexergetEmbeddedToken(Token* returning_token, Lexer* lexer);

}