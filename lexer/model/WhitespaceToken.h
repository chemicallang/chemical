//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

#include <utility>

class WhitespaceToken : public LexToken {
public:

    WhitespaceToken(int start, int end) : LexToken(start, end) {

    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Whitespace:");
        buf.append(std::to_string(this->end - this->start));
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};