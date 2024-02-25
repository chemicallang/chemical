// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <memory>
#include "lexer/Lexer.h"

std::optional<int> Lexer::lexInt(bool intOnly) {
    char current = provider.peek();
    char next = provider.peek(1);
    if ((current >= '0' && current <= '9') || (current == '-' && next >= '0' && next <= '9')) {
        bool isNegative = false;
        if (current == '-') {
            provider.readCharacter();
            isNegative = true;
        }
        current = provider.readCharacter(); // this line reads first digit
        int digits = current - '0'; // stores first digit as
        while (!provider.eof()) {
            current = provider.peek();
            if (current >= '0' && current <= '9') {
                current = provider.readCharacter();
                digits *= 10;
                digits += current - '0';
            } else if (!intOnly && current == '.') {
                // TODO handle floating point
                provider.readCharacter();
            } else {
                break;
            }
        }
        if (isNegative) {
            return -digits;
        } else {
            return digits;
        }
    }
    return std::nullopt;
}

bool Lexer::lexIntToken() {
    auto prevPos = position();
    auto lexedInt = lexInt();
    if (lexedInt.has_value()) {
        tokens.emplace_back(std::make_unique<IntToken>(prevPos, provider.position() - prevPos.position, lexedInt.value()));
        return true;
    } else {
        return false;
    }
}

void Lexer::error(const std::string& message) {
    lexError = {provider.getStreamPosition(), path, message};
}