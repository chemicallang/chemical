// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/statements/Typealias.h"

lex_ptr<TypealiasStatement> Parser::parseTypealiasStatement() {
    if(consume("typealias")) {
        auto from = parseType();
        if(from.has_value()) {
            if(consume_op('=')) {
                auto to = parseType();
                if (to.has_value()) {
                    return std::make_unique<TypealiasStatement>(std::move(from.value()), std::move(to.value()));
                } else {
                    error("expected a type after '=' in typealias statement");
                }
            } else {
                error("expected '=' sign after type in typealias statement");
            }
        } else {
            error("expected a type after typealias");
        }
    } else {
        return std::nullopt;
    }
}