// Copyright (c) Qinetik 2024.

#pragma once

#include "lexer/model/LexTokenType.h"
#include <string>

class LexToken;

class Position;

class ASTNode;

class CSTVisitor;

class CompoundCSTToken;

class CSTToken {
public:

    /**
     * the type of token
     */
    LexTokenType tok_type;

    /**
     * default constructor
     */
    explicit CSTToken(LexTokenType tok_type) : tok_type(tok_type) {
        // do nothing
    }

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
    void accept(CSTVisitor *visitor);

    /**
     * get lex token type of this token
     */
    inline LexTokenType type() const {
        return tok_type;
    }

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
     * this can be a union definition, unnamed union also uses the same type
     */
    bool is_union_def() {
        return type() == LexTokenType::CompUnionDef;
    }

    /**
     * this can be a union definition, unnamed union also uses the same type
     */
    bool is_struct_def() {
        return type() == LexTokenType::CompStructDef;
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
        return is_comp_type() || type() == LexTokenType::Type;
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
        return is_comp_value() || is_primitive_var() || type() == LexTokenType::CompMacro;
    }

    /**
     * The start position of the token, for debugging purposes
     */
    Position start() const;

    /**
     * this is a debug type string, that is given by each token
     */
    std::string type_string() const;

    /**
     * default destructor
     */
    virtual ~CSTToken() = default;

};