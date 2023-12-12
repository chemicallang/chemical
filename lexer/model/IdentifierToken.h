//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

#include <utility>

class IdentifierToken : public LexToken {
public:

    std::string identifier;

    IdentifierToken(int start, std::string identifier) : LexToken(start, identifier.length()), identifier(std::move(identifier)) {

    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Identifier:");
        buf.append(this->identifier);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};