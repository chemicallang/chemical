// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "core/diag/Position.h"
#include "core/source/SourceLocation.h"
#include "lexer/Token.h"
#include <vector>

class LocationManager;

class CaretPositionAnalyzer {
public:

    /**
     * location manager is required to decode locations
     */
    LocationManager& loc_man;

    /**
     * This is the position of the cursor in the document
     * The first indicates the line number (zero based)
     * The second indicates the character number (also zero based)
     */
    Position caret_position;

    /**
     * constructor
     */
    CaretPositionAnalyzer(LocationManager& loc_man, Position position) : loc_man(loc_man), caret_position(position) {

    }

    /**
     * will return true, if given position is ahead of caret position
     * @deprecated
     */
    inline bool is_ahead(const Position& position) const {
        return position.is_ahead(caret_position);
    }

    /**
     * will return true, if given position is behind caret position
     * @deprecated
     */
    inline bool is_behind(const Position& position) const {
        return position.is_behind(caret_position);
    }

    /**
     * is equal to caret position
     * @deprecated
     */
    inline bool is_eq_caret(const Position& position) const {
        return position.is_equal(caret_position);
    }

    /**
     * will return true, if given position is ahead of caret position
     */
    bool is_ahead(Token* token) const {
        return is_ahead(token->position);
    }

    /**
     * is token position equal to caret position
     */
    bool is_eq_caret(Token* token) const {
        return is_eq_caret(token->position);
    }

    /**
     * is the cursor ahead of the given token
     */
    inline bool is_caret_ahead(Token* token) const {
        return !is_ahead(token);
    }

    /**
     * is the cursor ahead of the given position
     */
    inline bool is_caret_ahead(const Position& position) const {
        return !is_ahead(position);
    }

    /**
     * is the cursor / caret behind of the given token
     */
    inline bool is_caret_behind(Token* token) const {
        return is_ahead(token);
    }

    /**
     * is caret equal or behind the token's position
     */
    inline bool is_caret_eq_or_behind(Token* token) const {
        return is_ahead(token) || is_eq_caret(token);
    }

    /**
     * check if caret is inside given locations
     */
    inline bool is_caret_inside(const Position& start, const Position& end) {
        return is_behind(start) && !is_behind(end);
    }

    /**
     * check if the caret is inside the given source location
     */
    bool is_caret_inside(SourceLocation location);

    /**
     * check if caret is inside this token
     */
    bool is_caret_inside(Token* token) {
        // TODO this check is a little unreliable
        return is_behind(token->position) && !is_behind({
            token->position.line,
            static_cast<unsigned int>(token->position.character + token->value.size())
        });
    }

    /**
     * gets index of the token which is right before caret
     *
     * also assumes that caret is present inside these tokens
     * otherwise -1 if caret is behind all tokens, -2 if ahead of all tokens
     */
    Token* token_before_caret(std::vector<Token> &tokens);

    /**
     * chain before caret
     * find's the access chain before the caret position, the returned token
     * can be nullptr or has type CompAccessChain, CompAccessChainNode
     */
    Token* chain_before_caret(std::vector<Token> &tokens);

};