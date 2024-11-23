// Copyright (c) Qinetik 2024.

#pragma once

#include "stream/SourceProvider.h"
#include "Token.h"
#include "LexUnit.h"
#include "std/chem_string.h"
#include "MultiStrAllocator.h"

class CompilerBinder;

/**
 * the lexer is used to tokenize tokens
 */
class Lexer {
public:

    /**
     *
     */
    MultiStrAllocator allocator;

    /**
     * the path to the file we are lexing
     */
    std::string file_path;

    /**
     * the source provider is used to read the source
     */
    SourceProvider& provider;

    /**
     * the binder that will be used to compile binding code
     * if not present, cbi is considered disabled
     */
    CompilerBinder* const binder;

    /**
     * the constructor
     */
    Lexer(
            std::string file_path,
            SourceProvider &provider,
            CompilerBinder* binder = nullptr
    );

    /**
     * the function to get the next token
     */
    Token getNextToken();

    /**
     * get the entire unit, lexing whole file at once
     */
    void getUnit(LexUnit& outUnit);

};