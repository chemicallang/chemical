// Copyright (c) Qinetik 2024.

#include "LexerCBI.h"
#include "lexer/Lexer.h"

BatchAllocator* LexergetFileAllocator(Lexer* lexer) {
    return &lexer->file_allocator;
}