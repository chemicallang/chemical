// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/CommentToken.h"
#include "lexer/model/tokens/MultilineCommentToken.h"

lex_ptr<Comment> Parser::parseComment() {
    auto comment = consumeOfType<CommentToken>(LexTokenType::Comment, false);
    if(comment != nullptr) {
        return std::make_unique<Comment>(comment->value, false);
    } else {
        auto multi = consumeOfType<MultilineCommentToken>(LexTokenType::MultilineComment, false);
        if(multi != nullptr) {
            return std::make_unique<Comment>(multi->value, true);
        }
    }
    return std::nullopt;
}