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
     * deleted copy constructor
     */
    CSTToken(const CSTToken& other) = delete;

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
    [[nodiscard]]
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
    [[nodiscard]]
    inline LexTokenType type() const {
        return tok_type;
    }

    /**
     * is token a compound token, meaning token holds multiple children
     */
    [[nodiscard]]
    bool compound() const {
        return type() >= LexTokenType::IndexCompStart && type() <= LexTokenType::IndexCompEnd;
    }

    /**
     * get the token as a compound token
     */
    [[nodiscard]]
    inline CompoundCSTToken* as_compound() const {
        return (CompoundCSTToken*) this;
    }

    /**
     * get the token as a lex token
     */
    [[nodiscard]]
    inline LexToken* as_lex_token() const {
        return (LexToken*) this;
    }

    /**
     * is the token var init compound token
     */
    [[nodiscard]]
    bool is_var_init() const {
        return type() == LexTokenType::CompVarInit;
    }

    /**
     * check if this token is a compound function declaration
     */
    [[nodiscard]]
    bool is_func_decl() const {
        return type() == LexTokenType::CompFunction;
    }

    /**
     * this can be a union definition, unnamed union also uses the same type
     */
    [[nodiscard]]
    bool is_union_def() const {
        return type() == LexTokenType::CompUnionDef;
    }

    /**
     * this can be a union definition, unnamed union also uses the same type
     */
    [[nodiscard]]
    bool is_struct_def() const {
        return type() == LexTokenType::CompStructDef;
    }

    /**
     * check if this token is a compound variant
     */
    [[nodiscard]]
    bool is_variant() const {
        return type() == LexTokenType::CompVariant;
    }

    /**
     * check if this token is a compound variant member
     */
    [[nodiscard]]
    bool is_variant_member() const {
        return type() == LexTokenType::CompVariantMember;
    }

    /**
     * is a ref token
     */
    [[nodiscard]]
    bool is_ref() const {
        return type() == LexTokenType::Variable || type() == LexTokenType::Type;
    }

    /**
     * check if this cst token is a struct value
     */
    [[nodiscard]]
    bool is_struct_value() const {
        return type() == LexTokenType::CompStructValue;
    }

    /**
     * check if its a compound type
     */
    [[nodiscard]]
    bool is_comp_type() const {
        return type() >= LexTokenType::IndexCompTypeStart && type() <= LexTokenType::IndexCompTypeEnd;
    }

    /**
     * check if token is a identifier
     */
    [[nodiscard]]
    bool is_identifier() const {
        return type() == LexTokenType::Identifier;
    }

    /**
     * check if its a type
     */
    [[nodiscard]]
    bool is_type() const {
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
    [[nodiscard]]
    Position start() const;

    /**
     * this is a debug type string, that is given by each token
     */
    [[nodiscard]]
    std::string type_string() const;

    /**
     * default destructor
     */
    virtual ~CSTToken() = default;

};