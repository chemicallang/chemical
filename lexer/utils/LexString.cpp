// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/IdentifierToken.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/StringToken.h"

bool Lexer::lexStringToken() {
    if(provider.increment('"')) {
        std::string str;
        while(!provider.eof() && !lexError.has_value()){
            auto readChar = provider.peek();
            if(readChar == '"') {
                provider.readCharacter();
                tokens.emplace_back(std::make_unique<StringToken>(backPosition(str.length() + 2), str));
                return true;
            } else if(readChar == '\\') {
                provider.readCharacter();
                str += escape_sequence(provider.readCharacter());
            } else {
                provider.readCharacter();
                str += readChar;
            }
        }
    }
    return false;
}