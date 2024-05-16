// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LexToken.h"

class AnnotationToken : public LexToken {
public:

    AnnotationToken(
            const Position &position,
            std::string value
    ) : LexToken(position, std::move(value)) {
        // nothing
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitAnnotationToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Annotation;
    }

};