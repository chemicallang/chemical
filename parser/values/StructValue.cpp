// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/values/StructValue.h"

lex_ptr<StructValue> Parser::parseStructValue(const std::string& structName) {

    if(consume_op('{')) {

        std::unordered_map<std::string, std::unique_ptr<Value>> values;

        do {

            auto identifier = consume_identifier(false);
            if(identifier.has_value()) {

                if(consume_op(':')) {

                    auto value = parseExpression();

                    if(value.has_value()) {

                        values[identifier.value()] = std::move(value.value());

                    } else {
                        error("expected an expression after the ':' for struct member");
                    }

                } else {
                    error("expected a ':' after the identifier for struct member");
                }

                consume_op(',');

            } else {
                break;
            }

        } while(true);

        if(!consume_op('}')) {
            error("expected a '}' in struct value initialization");
        }

        return std::make_unique<StructValue>(structName, std::move(values));

    }

    return std::nullopt;

}