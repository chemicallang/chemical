// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/CommentToken.h"
#include "lexer/model/tokens/MultilineCommentToken.h"

lex_ptr<Comment> Parser::parseComment() {
    auto comment = consumeOfType<CommentToken>(LexTokenType::Comment, false);
    if(comment.has_value()) {
        return std::make_unique<Comment>(comment.value()->value, false);
    } else {
        auto multi = consumeOfType<MultilineCommentToken>(LexTokenType::MultilineComment, false);
        if(multi.has_value()) {
            return std::make_unique<Comment>(multi.value()->value, true);
        }
    }
    return std::nullopt;
}