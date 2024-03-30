// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "lexer/model/tokens/TypeToken.h"
#include "ast/types/Int32Type.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/GenericType.h"
#include "ast/types/ArrayType.h"
#include "ast/values/IntValue.h"

lex_ptr<BaseType> Parser::parseType() {
    if (token_type() == LexTokenType::Type) {
        auto type = consume<TypeToken>()->value;
        auto type_fn = primitive_type_map.find(type);
        auto base_type = type_fn != primitive_type_map.end() ? (
                std::unique_ptr<BaseType>(type_fn->second(this))
        ) : std::make_unique<ReferencedType>(type);
        if(consume_op('<')) {
            if(type_fn == primitive_type_map.end()) {
                auto child_type = parseType();
                if(child_type.has_value()) {
                    base_type = std::make_unique<GenericType>(type, std::move(child_type.value()));
                } else {
                   error("expected a type after '<' for generic type");
                }
            } else {
                error("primitive type is not a generic : " + type);
            }
            if(!consume_op('>')) {
                error("expected '>' for generic type");
            }
        } else if(consume_op('[')) {
            auto number = parseIntValue();
            auto size = number.has_value() ? number.value()->as_int() : -1;
            base_type = std::make_unique<ArrayType>(std::move(base_type), size);
            if(!consume_op(']')) {
                error("expected ']' for array type");
            }
        }
        if (consume_op('*')) {
            return std::make_unique<PointerType>(std::move(base_type));
        } else {
            return base_type;
        }
    } else {
        return std::nullopt;
    }
}