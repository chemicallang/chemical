// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LexToken.h"

class ModifiableToken : LexToken {

    unsigned modifiers = 0;

    unsigned int lsp_modifiers() override {
        return modifiers;
    }

};