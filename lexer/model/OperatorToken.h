//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

#include <utility>

class CharOperatorToken : public LexToken {
public:

    char op;

    CharOperatorToken(unsigned int start, unsigned int length, unsigned int lineNumber, char op) : LexToken(start, length, lineNumber), op(op) {

    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_operator;
    }

    [[nodiscard]] std::string type_string() const override {
        return &"Operator:" [ op];
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};