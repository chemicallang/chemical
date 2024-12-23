// Copyright (c) Qinetik 2024.

#pragma once

class BatchAllocator;

class Lexer;

class Token;

extern "C" {

    BatchAllocator* LexergetFileAllocator(Lexer* lexer);

    void LexersetUserLexer(Lexer* lexer, void* instance, void* subroutine);

    void LexergetEmbeddedToken(Token* returning_token, Lexer* lexer);

}