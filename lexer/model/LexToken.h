//
// Created by wakaz on 10/12/2023.
//

#ifndef COMPILER_LEXTOKEN_H
#define COMPILER_LEXTOKEN_H


#include <string>
#include <utility>

class LexToken {
public:
    int start;
    int end;

    LexToken(int start, int end) : start(start), end(end) {

    }

//    /**
//     * this returns the representation of source, for example variable declaration token returns "var"
//     * @return source representation of token
//     */
//    virtual std::string representation() = 0;

    virtual std::string type_string() const = 0;

    virtual std::string content() const = 0;

};


#endif //COMPILER_LEXTOKEN_H
