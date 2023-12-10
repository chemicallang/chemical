//
// Created by wakaz on 10/12/2023.
//

#include "LexToken.h"

class IntToken : public LexToken {
public:

    int value;

    IntToken(int start, int end, int value) : LexToken(start, end), value(value) {

    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Int:");
        buf.append(std::to_string(value));
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return std::to_string(value);
    }

};