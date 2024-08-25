// Copyright (c) Qinetik 2024.

#pragma once

#include "CSTToken.h"

/**
 * class allows to take ownership of the allocated tokens during the lexing process
 */
class CSTUnit {
public:

    /**
     * the size by which tokens vector are allocated (1000 tokens, if they expire we allocate another 1000)
     */
    static constexpr unsigned int VEC_SIZE = 1000;

    /**
     * these are all the flat lex tokens, allocated during lexing process
     */
    std::vector<std::vector<CSTToken>> allocated_lex_tokens;

    /**
     * these are all the compound tokens, allocated during lexing process
     */
    std::vector<std::vector<CSTToken>> allocated_compound_tokens;

    /**
     * these contain references to tokens, these are organized tokens, compound and lex together
     * can be visited using a visitor
     */
    std::vector<CSTToken*> tokens;

    /**
     * allocate a vector into given tokens of size VEC_SIZE
     */
    static std::vector<CSTToken>& allocate_last(std::vector<std::vector<CSTToken>>& vec, unsigned int size) {
        vec.emplace_back();
        auto& last = vec.back();
        vec.back().reserve(size);
        return last;
    }

    /**
     * a helper function
     */
    static inline std::vector<CSTToken>& allocate_last(std::vector<std::vector<CSTToken>>& vec) {
        return allocate_last(vec, VEC_SIZE);
    }

    /**
     * initializes both vectors
     */
    inline void init() {
        allocate_last(allocated_lex_tokens);
        allocate_last(allocated_compound_tokens);
        tokens.reserve(200);
    }

    /**
     * get's the current vector of tokens that should be used to emplace tokens
     */
    inline std::vector<CSTToken>& get_current(std::vector<std::vector<CSTToken>>& vec) {
        auto& last = vec.back();
        if(last.size() == VEC_SIZE) {
            return allocate_last(vec);
        } else {
            return last;
        }
    }

    /**
     * emplace a lex token tokens vector
     */
    template<typename... Args>
    inline constexpr void emplace_tok(std::vector<std::vector<CSTToken>>& con, Args&&... args) {
        auto& vec = get_current(con);
        vec.emplace_back(std::forward<Args>(args)...);
        tokens.emplace_back(&vec.back());
    }

    /**
     * emplace a lex token tokens vector
     */
    template<typename... Args>
    inline constexpr void emplace(Args&&... args) {
        emplace_tok(allocated_lex_tokens, std::forward<Args>(args)...);
    }

    /**
     * emplace a token into
     */
    template<typename... Args>
    inline constexpr void emplace_compound(Args&&... args) {
        emplace_tok(allocated_compound_tokens, std::forward<Args>(args)...);
    }

    /**
     * resets a vector of vector of tokens
     */
    static void reset_vec(std::vector<std::vector<CSTToken>>& con) {
        if(con.empty()) {
            allocate_last(con);
        } else {
            con.erase(con.begin(), con.end() - 1);
            con.back().clear();
        }
    }

    /**
     * clear all the tokens, basically reset this unit
     */
    void reset() {
        reset_vec(allocated_lex_tokens);
        reset_vec(allocated_compound_tokens);
        tokens.clear();
    }

    /**
     * default constructor
     */
    CSTUnit() = default;

    /**
     * deleted copy constructor
     */
    CSTUnit(const CSTUnit& other) = delete;

    /**
     * default move constructor
     */
    CSTUnit(CSTUnit&& other) = default;

    /**
     * default destructor
     */
    ~CSTUnit() = default;

};