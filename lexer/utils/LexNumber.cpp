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
    std::string number;
    provider.readAnyNumber(number);
    if (!number.empty()) {
        switch(provider.peek()) {
            case 'f':
            case 'F':
            case 'l':
            case 'L':
                number += provider.readCharacter();
                break;
            case 'i':
            case 'u':
                // i16, i32, i64, i128
                // u16, u32, u64, u128
                number += provider.readCharacter();
                provider.readNumber(number);
                break;
        }
        emplace(LexTokenType::Number, backPosition(number.length()), number);
        return true;
    } else {
        return false;
    }
}