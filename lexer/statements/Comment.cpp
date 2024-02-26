// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CommentToken.h"
#include "lexer/model/tokens/MultilineCommentToken.h"

bool Lexer::lexSingleLineCommentTokens() {
    if(provider.increment("//")) {
        auto comment = lexAnything([&] () -> bool {
            return !hasNewLine();
        });
        tokens.emplace_back(std::make_unique<CommentToken>(backPosition(comment.length() + 2), comment));
        return true;
    } else return false;
}

bool Lexer::lexMultiLineCommentTokens() {
    if(provider.increment("/*")) {
        auto comment = lexAnything([&] () -> bool {
            return !provider.increment("*/");
        });
        tokens.emplace_back(std::make_unique<MultilineCommentToken>(backPosition(comment.length() + 4), comment));
        return true;
    } else {
        return false;
    }
}