// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "AbstractStringToken.h"

class MacroToken : public AbstractStringToken {
public:

    MacroToken(
            const Position &position,
            std::string value
    ) : AbstractStringToken(position, std::move(value)) {
        // nothing
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Macro;
    }

    std::string type_string() const override {
        return "Macro:" + value;
    }

};