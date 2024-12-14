// Copyright (c) Qinetik 2024.

#pragma once

#include "stream/SourceProvider.h"
#include "Token.h"
#include "LexUnit.h"
#include "std/alloc/SerialStrAllocator.h"
#include "compiler/cbi/Model.h"

class CompilerBinder;

namespace chem {
    class string;
}

/**
 * our state has two limits
 * 1 - all options default value must be false
 * 2 - we are only allowed 9 booleans, which can be encoded
 * in 9 bits inside the final format of 32 bit integer we will use
 * to represent our lexer state
 */
struct LexerState {

    /**
     * if lexer is operating under other mode not normal
     * for example comment mode, string mode which allows to collect tokens
     * as comments or strings, even symbols or operators
     */
    bool other_mode = false;

    /**
     * lexer will try to collect next character as a character token
     * or an escape sequence
     */
    bool char_mode = false;

    /**
     * lexer will try to collect next string until quotes as a string
     * if escape quotes then multiple strings are made
     */
    bool string_mode = false;

    /**
     * are we inside a multi line comment mode
     */
    bool comment_mode = false;

    /**
     * is user mode means a user lexer or a self provided user
     * lexer is being used, which is just a function pointer that
     * will provide the next token, this boolean can be encoded
     * in the user id part of the final state integer
     */
    bool user_mode = false;

};

/**
 * the lexer is used to tokenize tokens
 */
class Lexer : public LexerState {
public:

    /**
     * source provide helps in reading source
     */
    SourceProvider provider;

    /**
     * user lexer is the lexer we will activate upon encountering a
     * macro which starts with a hash symbol
     */
    UserLexerGetNextTokenFn user_lexer;

    /**
     * the allocator used for strings found in the source code
     */
    SerialStrAllocator str;

    /**
     * the path to the file we are lexing
     */
    std::string file_path;

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
            InputSource* source,
            CompilerBinder* binder = nullptr
    );

    /**
     * the function to get the next token
     */
    Token getNextToken();

    /**
     * get the tokens by lexing whole file at once
     */
    void getTokens(std::vector<Token>& tokens);

    /**
     * get the entire unit, lexing whole file at once
     */
    void getUnit(LexUnit& outUnit);

};

void to_string(chem::string& strOut, std::vector<Token>& tokens);