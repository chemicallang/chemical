// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class BoolToken : public AbstractStringToken {
public:

    BoolToken(const Position& position, std::string value) : AbstractStringToken(position, std::move(value)) {

    }

    LexTokenType type() const override {
        return LexTokenType::Bool;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]] std::string type_string() const override {
        return "Bool:" + value;
    }

};