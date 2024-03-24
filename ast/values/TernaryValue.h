// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

class TernaryValue : public Value {

    bool primitive() override {
        return false;
    }

};