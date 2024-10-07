// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <memory>
#include "lexer/Lexer.h"

bool Lexer::lexUnsignedIntAsNumberToken() {
    auto number = provider.readUnsignedInt();
    if (!number.empty()) {
        emplace(LexTokenType::Number, backPosition(number.length()), number);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexNumberToken() {
    if(!provider.is_peak_number_char()) {
        return false;
    }
    auto number = provider.readNumber();
    if (!number.empty()) {
        switch(provider.peek()) {
            case 'f':
            case 'F':
            case 'l':
            case 'L':
                number += provider.readCharacter();
        }
        emplace(LexTokenType::Number, backPosition(number.length()), number);
        return true;
    } else {
        return false;
    }
}