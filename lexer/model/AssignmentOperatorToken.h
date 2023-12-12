//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

#include <utility>

class AssignmentOperatorToken : public LexToken {
public:

    AssignmentOperatorToken(int start, int end) : LexToken(start, end) {

    }

    [[nodiscard]] std::string type_string() const override {
        return "AssignmentOperator:=";
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};