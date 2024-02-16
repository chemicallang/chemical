//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

#include <utility>

class KeywordToken : public LexToken {
public:

    std::string keyword;

    KeywordToken(unsigned int start, unsigned int end, unsigned int lineNumber, std::string keyword) : LexToken(start, end, lineNumber), keyword(std::move(keyword)) {

    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_keyword;
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