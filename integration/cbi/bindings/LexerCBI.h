// Copyright (c) Qinetik 2024.

#pragma once

class BatchAllocator;

class Lexer;

extern "C" {

    BatchAllocator* LexergetFileAllocator(Lexer* lexer);

}