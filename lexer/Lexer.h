// Copyright (c) Qinetik 2024.

#pragma once

#include "stream/SourceProvider.h"
#include "Token.h"
#include "LexUnit.h"
#include "std/chem_string.h"

class CompilerBinder;

/**
 * the lexer is used to tokenize tokens
 */
class Lexer {

    /**
     * the path to the file we are lexing
     */
    std::string file_path;

    /**
     * a single multi string is used for all strings found
     */
    chem::string multi_str;

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
     * a single char is put on multi string and pointer is returned
     */
    char* putSingleChar(char c);

    /**
     * the function to get the next token
     */
    Token getNextToken();

    /**
     * get the entire unit, lexing whole file at once
     */
    void getUnit(LexUnit& outUnit);

};