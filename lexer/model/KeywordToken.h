//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

#include <utility>

class KeywordToken : public LexToken {
public:

    std::string keyword;

    KeywordToken(int start, int end, std::string keyword) : LexToken(start, end), keyword(std::move(keyword)) {

    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Keyword:");
        buf.append(this->keyword);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};