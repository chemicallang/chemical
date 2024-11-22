// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexSingleLineCommentTokens() {
    if(provider.increment("//")) {
        std::string comment = "//";
        while(!provider.eof() && !hasNewLine()) {
            comment.append(1, provider.readCharacter());
        }
        emplace(LexTokenType::Comment, backPosition(comment.length()), comment);
        return true;
    } else return false;
}

bool Parser::lexMultiLineCommentTokens() {
    if(provider.increment("/*")) {
        const auto savedPosition = Position { provider.lineNumber, provider.lineCharacterNumber - 2 };
        std::string comment = "/*";
        while(!provider.eof() && !provider.increment("*/")) {
            comment.append(1, provider.readCharacter());
        }
        comment.append("*/");
        emplace(LexTokenType::MultilineComment, savedPosition, comment);
        return true;
    } else {
        return false;
    }
}