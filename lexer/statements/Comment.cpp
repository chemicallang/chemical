// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CommentToken.h"
#include "lexer/model/tokens/MultilineCommentToken.h"

bool Lexer::lexSingleLineCommentTokens() {
    if(provider.increment("//")) {
        std::string comment = "//";
        while(!provider.eof() && !hasNewLine()) {
            comment.append(1, provider.readCharacter());
        }
        tokens.emplace_back(std::make_unique<CommentToken>(backPosition(comment.length()), comment));
        return true;
    } else return false;
}

bool Lexer::lexMultiLineCommentTokens() {
    if(provider.increment("/*")) {
        std::string comment = "/*";
        while(!provider.eof() && !provider.increment("*/")) {
            comment.append(1, provider.readCharacter());
        }
        comment.append("*/");
        tokens.emplace_back(std::make_unique<MultilineCommentToken>(backPosition(comment.length()), comment));
        return true;
    } else {
        return false;
    }
}