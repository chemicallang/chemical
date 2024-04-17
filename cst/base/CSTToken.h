// Copyright (c) Qinetik 2024.

#pragma once

#include "CSTVisitor.h"
#include "lexer/model/LexTokenType.h"
#include <string>

class LexToken;

class Position;

class ASTNode;

class CSTToken {
public:

    /**
     * default constructor
     */
    CSTToken() = default;

    /**
     * get a pointer to the start lex token
     */
    virtual LexToken *start_token() = 0;

    /**
     * get a pointer to the end lex token
     */
    virtual LexToken *end_token() = 0;

    /**
     * every token must append its representation to this string
     */
    virtual void append_representation(std::string &rep) const = 0;

    /**
     * implement the visitor pattern
     */
    virtual void accept(CSTVisitor *visitor) = 0;

    /**
     * get lex token type of this token
     */
    virtual LexTokenType type() const = 0;

    /**
     * is token a compound token, meaning token holds multiple children
     */
    bool compound() {
        return type() >= LexTokenType::IndexCompStart && type() <= LexTokenType::IndexCompEnd;
    }

    /**
     * is the token var init compound token
     */
    bool is_var_init() {
        return type() == LexTokenType::CompVarInit;
    }

    /**
     * check if this cst token is a struct value
     */
    bool is_struct_value() {
        return type() == LexTokenType::CompStructValue;
    }

#ifdef DEBUG

    /**
     * The start position of the token, for debugging purposes
     */
    virtual Position start() = 0;

    /**
     * this is a debug type string, that is given by each token
     */
    virtual std::string type_string() const = 0;

#endif

    /**
     * default destructor
     */
    virtual ~CSTToken() = default;

};