//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

#include <utility>

class SemiColonToken : public LexToken {
public:

    SemiColonToken(int start, int end) : LexToken(start, end) {

    }

    [[nodiscard]] std::string type_string() const override {
        return "SemiColon:;";
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};