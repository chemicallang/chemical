// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/structures/TryCatch.h"

lex_ptr<TryCatch> Parser::parseTryCatchBlock() {
    if(consume("try")) {
        if(consume_op('{')) {
            auto tryScope = parseScope();
            consume_op('}');
            std::optional<Scope> catchScope = std::nullopt;
            catch_var_type catchVar = std::nullopt;
            if (consume("catch")) {
                if(consume_op('(')) {
                    auto id = consume_identifier();
                    if(id.has_value()) {
                        consume_op(':');
                        auto type = parseType();
                        if (type.has_value()) {
                            catchVar = std::pair(id.value(), std::move(type.value()));
                        }
                        consume_op(')');
                    }
                }
                if(consume_op('{')) {
                    catchScope.emplace(parseScope());
                    consume_op('{');
                }
            }
            return std::make_unique<TryCatch>(std::move(tryScope), std::move(catchVar), std::move(catchScope));
        }
    }
    return std::nullopt;
}