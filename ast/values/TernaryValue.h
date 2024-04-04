// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

class TernaryValue : public Value {

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    bool primitive() override {
        return false;
    }

};