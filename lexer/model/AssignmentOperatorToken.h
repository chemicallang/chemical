//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

#include <utility>

class AssignmentOperatorToken : public LexToken {
public:

    AssignmentOperatorToken(unsigned int start, unsigned int length, unsigned int lineNumber) : LexToken(start, length, lineNumber) {

    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_operator;
    }

    [[nodiscard]] std::string type_string() const override {
        return "AssignmentOperator:=";
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};