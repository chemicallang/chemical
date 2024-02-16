//
// Created by wakaz on 10/12/2023.
//

#ifndef COMPILER_LEXTOKEN_H
#define COMPILER_LEXTOKEN_H


#include <string>
#include <utility>
#include "minLsp/SemanticTokens.h"

class LexToken {
public:
    unsigned int start;
    unsigned int length;
    unsigned int lineNumber;

    LexToken(unsigned int start, unsigned int length, unsigned int lineNumber) : start(start), length(length), lineNumber(lineNumber) {

    }

//    /**
//     * this returns the representation of source, for example variable declaration token returns "var"
//     * @return source representation of token
//     */
//    virtual std::string representation() = 0;

    inline unsigned int end() {
        return start + length;
    }

    virtual LspSemanticTokenType lspType() const = 0;

    virtual std::string type_string() const = 0;

    virtual std::string content() const = 0;

};


#endif //COMPILER_LEXTOKEN_H
