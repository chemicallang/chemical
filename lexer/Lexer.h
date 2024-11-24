// Copyright (c) Qinetik 2024.

#pragma once

#include "stream/SourceProvider.h"
#include "Token.h"
#include "LexUnit.h"
#include "MultiStrAllocator.h"

class CompilerBinder;

namespace chem {
    class string;
}

/**
 * lexer state is just boolean struct
 * which is limited to 32 bits at the moment
 * all options default value must be false
 */
struct LexerState {

    /**
     * if lexer is operating under other mode not normal
     * for example comment mode, string mode which allows to collect tokens
     * as comments or strings, even symbols or operators
     */
    bool other_mode;

    /**
     * lexer has detected a single line comment
     */
    bool comment_mode;

};

/**
 * the lexer is used to tokenize tokens
 */
class Lexer : public LexerState {
public:

    /**
     * the allocator used for strings found in the source code
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

void to_string(chem::string& strOut, std::vector<Token>& tokens);