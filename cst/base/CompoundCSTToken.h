// Copyright (c) Qinetik 2024.

#pragma once

#include "CSTToken.h"
#include <utility>
#include <vector>
#include <memory>
#include "common/Position.h"

class CompoundCSTToken : public CSTToken {
public:

    /**
     * cst tokens vecotr
     */
    std::vector<std::unique_ptr<CSTToken>> tokens;

    /**
     * the constructor for CompoundCSTToken
     */
    CompoundCSTToken(std::vector<std::unique_ptr<CSTToken>> tokens) : tokens(std::move(tokens)) {
        tokens.shrink_to_fit();
    }

#ifdef DEBUG

    LexToken *start_token() override {
        if (tokens[0]->compound()) {
            return tokens[0]->start_token();
        } else {
            return (LexToken *) (tokens[0].get());
        }
    }

    LexToken *end_token() override {
        auto last = tokens.size() - 1;
        if (tokens[last]->compound()) {
            return tokens[last]->end_token();
        } else {
            return (LexToken *) (tokens[last].get());
        }
    }

    Position start() override {
        return tokens[0]->start();
    }

    virtual std::string compound_type_string() const = 0;

    std::string type_string() const override {
        std::string ret(compound_type_string());
        ret.append(1, '[');
        unsigned i = 0;
        unsigned size = tokens.size();
        while (i < size) {
            ret.append(tokens[i]->type_string());
            if (i < size - 1) ret.append(1, ',');
            i++;
        }
        ret.append(1, ']');
        return ret;
    }

#endif

    /**
     * appends representation of the compound CST token to rep string
     */
    void append_representation(std::string &rep) const override {
        for (const auto &tok: tokens) {
            tok->append_representation(rep);
        }
    }

};