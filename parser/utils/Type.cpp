// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "lexer/model/tokens/TypeToken.h"
#include "ast/types/Int32Type.h"
#include "ast/types/StringType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/AnyType.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/GenericType.h"

lex_ptr<BaseType> Parser::parseType() {
    if (token_type() == LexTokenType::Type) {
        auto type = consume<TypeToken>()->value;
        auto type_fn = primitive_type_map.find(type);
        auto base_type = type_fn != primitive_type_map.end() ? (
                std::unique_ptr<BaseType>(type_fn->second(this))
        ) : std::make_unique<ReferencedType>(type);
        if(consume_op('<')) {
            if(type_fn == primitive_type_map.end()) {
                base_type = std::make_unique<GenericType>(type, std::move(base_type));
            } else {
                error("primitive type is not a generic : " + type);
            }
            if(!consume_op('>')) {
                error("expected '>' for generic type");
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