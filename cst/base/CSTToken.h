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
    LexToken *start_token();

    /**
     * get a pointer to the end lex token
     */
    LexToken *end_token();

    /**
     * every token must append its representation to this string
     */
    void append_representation(std::string &rep) const;

    /**
     * returns representation of the token
     */
    inline std::string representation() const {
        std::string rep;
        append_representation(rep);
        return rep;
    }

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
    bool compound() const {
        return type() >= LexTokenType::IndexCompStart && type() <= LexTokenType::IndexCompEnd;
    }

    /**
     * get the token as a compound token
     */
    inline CompoundCSTToken* as_compound() const {
        return (CompoundCSTToken*) this;
    }

    /**
     * get the token as a lex token
     */
    inline LexToken* as_lex_token() const {
        return (LexToken*) this;
    }

    /**
     * get the token as a variable
     */
    inline VariableToken* as_variable() const {
        return (VariableToken*) this;
    }

    /**
     * get the token as a ref token
     */
    inline RefToken* as_ref() {
        return (RefToken*) this;
    }

    /**
     * is the token var init compound token
     */
    bool is_var_init() {
        return type() == LexTokenType::CompVarInit;
    }

    /**
     * check if this token is a compound function declaration
     */
    bool is_func_decl() {
        return type() == LexTokenType::CompFunction;
    }

    /**
     * is a ref token
     */
    bool is_ref() {
        return type() == LexTokenType::Variable || type() == LexTokenType::Type;
    }

    /**
     * check if this cst token is a struct value
     */
    bool is_struct_value() {
        return type() == LexTokenType::CompStructValue;
    }

    /**
     * check if its a compound type
     */
    bool is_comp_type() {
        return type() >= LexTokenType::IndexCompTypeStart && type() <= LexTokenType::IndexCompTypeEnd;
    }

    /**
     * check if token is a identifier
     */
    bool is_identifier() {
        return type() == LexTokenType::Identifier;
    }

    /**
     * check if its a type
     */
    bool is_type() {
        return is_comp_type() && type() == LexTokenType::Type;
    }

    /**
     * check if its a compound value
     */
    [[nodiscard]]
    bool is_comp_value() const {
        return type() >= LexTokenType::IndexCompValueStart && type() <= LexTokenType::IndexCompValueEnd;
    }

    /**
     * check if its a primitive value
     */
    [[nodiscard]]
    bool is_primitive_value() const {
        auto t = type();
        return t == LexTokenType::Char || t == LexTokenType::String || t == LexTokenType::Bool ||
               t == LexTokenType::Number || t == LexTokenType::Null;
    }

    /**
     * check is a reference or variable
     */
    [[nodiscard]]
    bool is_primitive_var() const {
        return type() == LexTokenType::Variable || is_primitive_value();
    }

    /**
     * check if its a value
     */
    [[nodiscard]]
    bool is_value() const {
        return is_comp_value() || is_primitive_var();
    }

#ifdef DEBUG

    /**
     * The start position of the token, for debugging purposes
     */
    Position start() const;

    /**
     * this is a debug type string, that is given by each token
     */
    std::string type_string() const;

#endif

    /**
     * default destructor
     */
    virtual ~CSTToken() = default;

};