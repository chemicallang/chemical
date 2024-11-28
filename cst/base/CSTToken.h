// Copyright (c) Qinetik 2024.

#pragma once

#include "parser/model/LexTokenType.h"
#include "integration/common/Position.h"
#include <string>
#include <vector>

class Position;

class ASTNode;

class CSTVisitor;

class ASTAny;

class CSTToken {
public:

    /**
     * the type of token
     */
    LexTokenType tok_type;

    union {
        /**
         * this struct is used for non compound tokens
         */
        struct {
            Position position;
            std::string value;
        } flat;
        /**
         * this struct is used for compound tokens
         */
        std::vector<CSTToken*> tokens;
        /**
         * contains a value, node or type
         */
        struct {
            Position position;
            void* data_ptr;
        } straight;
    };

#ifdef LSP_BUILD
    /**
     * in lsp build, every token knows about it's ast any that it created
     * this helps us provide go to definition, hover things
     */
    ASTAny* any = nullptr;
#endif

    /**
     * constructor for lex token (non compound)
     */
    CSTToken(LexTokenType type, const Position& pos, std::string val) : tok_type(type) {
        new(&flat.position) Position(pos);
        new(&flat.value) std::string(std::move(val));
    }

    /**
     * constructor for compound tokens
     */
    explicit CSTToken(LexTokenType type) : tok_type(type) {
        new(&tokens) std::vector<CSTToken*>();
    }

    /**
     * token with type straight
     */
    CSTToken(LexTokenType type, void* data, const Position& pos) : tok_type(type) {
        new(&straight.position) Position(pos);
        straight.data_ptr = data;
    }

    /**
     * deleted copy constructor
     */
    CSTToken(const CSTToken& other) = delete;

    /**
     * a helper function
     */
    static inline bool isStraight(LexTokenType type) {
        return type == LexTokenType::StraightNode || type == LexTokenType::StraightType || type == LexTokenType::StraightValue;
    }

    /**
     * deleted move constructor
     */
    CSTToken(CSTToken&& other) {
        tok_type = other.tok_type;
        if(isStraight(tok_type)) {
            return;
        }
        if(other.compound()) {
            new(&tokens) std::vector<CSTToken*>(std::move(other.tokens));
        } else {
            flat.position = other.flat.position;
            new(&flat.value) std::string(std::move(other.flat.value));
        }
    }

    /**
     * get a pointer to the start lex token
     */
    CSTToken *start_token();

    /**
     * get a pointer to the end lex token
     */
    CSTToken *end_token();

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
     * get the position of this token
     */
    inline const Position& position() const {
        if(isStraight(tok_type)) {
            return straight.position;
        } else {
            return flat.position;
        }
    }

    /**
     * get the value of this token
     */
    inline const std::string& value() const {
        return flat.value;
    }

    /**
     * get the mutable value unsafely
     */
    inline std::string& unsafe_mutable_value() {
        return flat.value;
    }

    /**
     * line number of lex token
     */
    [[nodiscard]]
    inline unsigned int lineNumber() const {
        if(isStraight(tok_type)) {
            return straight.position.line;
        } else {
            return flat.position.line;
        }
    }

    /**
     * line character number of lex token
     */
    [[nodiscard]]
    inline unsigned int lineCharNumber() const {
        if(isStraight(tok_type)) {
            return straight.position.character;
        } else {
            return flat.position.character;
        }
    }

    /**
     * string length of the lex token
     */
    [[nodiscard]]
    inline unsigned int length() const {
        return flat.value.size();
    }

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
    [[deprecated]]
    inline CSTToken* as_compound() {
        return this;
    }

    /**
     * get the token as a lex token
     */
    [[nodiscard]]
    [[deprecated]]
    inline CSTToken* as_lex_token() {
        return this;
    }

    /**
     * get the token as a lex token
     */
    [[nodiscard]]
    [[deprecated]]
    inline CSTToken* as_ref() {
        return this;
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
     * check if it's compound value
     */
    inline static bool is_comp_value(LexTokenType type) {
        return type >= LexTokenType::IndexCompValueStart && type <= LexTokenType::IndexCompValueEnd;
    }

    /**
     * check if its a compound value
     */
    [[nodiscard]]
    bool is_comp_value() const {
        return is_comp_value(type());
    }

    /**
     * check if its a primitive value
     */
    [[nodiscard]]
    inline static bool is_primitive_value(LexTokenType t) {
        return t == LexTokenType::Char || t == LexTokenType::String || t == LexTokenType::Bool ||
               t == LexTokenType::Number || t == LexTokenType::Null;
    }

    /**
     * check if its a primitive value
     */
    [[nodiscard]]
    bool is_primitive_value() const {
        return is_primitive_value(type());
    }

    /**
     * check is a reference or variable
     */
    [[nodiscard]]
    inline static bool is_primitive_var(LexTokenType type) {
        return type == LexTokenType::Variable || is_primitive_value(type);
    }

    /**
     * check is a reference or variable
     */
    [[nodiscard]]
    bool is_primitive_var() const {
        return is_primitive_var(type());
    }

    /**
     * check if its a value
     */
    [[nodiscard]]
    inline static bool is_value(LexTokenType type) {
        return type == LexTokenType::StraightValue || is_comp_value(type) || is_primitive_var(type) || type == LexTokenType::CompMacro;
    }

    /**
     * check if its a value
     */
    [[nodiscard]]
    bool is_value() const {
        return is_value(type());
    }

    /**
     * The start position of the token, for debugging purposes
     */
    [[nodiscard]]
    const Position& start() const;

    /**
     * this is a debug type string, that is given by each token
     */
    [[nodiscard]]
    std::string type_string() const;

    /**
     * default destructor
     */
    ~CSTToken() {
        if(isStraight(tok_type)) {
            return;
        }
        if(compound()) {
            tokens.~vector();
        } else {
            flat.value.~basic_string();
        }
    }

};