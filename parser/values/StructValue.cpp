// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"

bool Parser::lexStructValueTokens(unsigned back_start) {
    if(lexOperatorToken(TokenType::LBrace)) {

        unsigned start = tokens_size() - 1 - back_start;

        // lex struct member value tokens
        do {
            lexWhitespaceAndNewLines();
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                storeIdentifier(id);
                lexWhitespaceToken();
                if(!lexOperatorToken(TokenType::ColonSym)) {
                    mal_value(start, "expected a ':' for initializing struct member " + std::string(id->value));
                    return true;
                }
                lexWhitespaceToken();
                if(!(lexExpressionTokens(true) || lexArrayInit())) {
                    mal_value(start, "expected an expression after ':' for struct member " + std::string(id->value));
                    return true;
                }
                lexWhitespaceToken();
                lexOperatorToken(TokenType::CommaSym);
            } else {
                break;
            }
        } while(true);

        lexWhitespaceAndNewLines();

        if(!lexOperatorToken(TokenType::RBrace)) {
            mal_value(start, "expected '}' for struct value");
            return true;
        }

        compound_from(start, LexTokenType::CompStructValue);

        return true;
    }
    return false;
}