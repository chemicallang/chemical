// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "lexer/model/tokens/TypeToken.h"
#include "ast/types/Int32Type.h"
#include "ast/types/StringType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"

lex_ptr<BaseType> Parser::parseType() {
    if (token_type() == LexTokenType::Type) {
        auto type = std::move(consume<TypeToken>()->value);
        if (type == "int") { // currently parsing int as int32
            return std::make_unique<Int32Type>();
        } else if (type == "string") {
            return std::make_unique<StringType>();
        } else if (type == "float") {
            return std::make_unique<FloatType>();
        } else if (type == "double") {
            return std::make_unique<DoubleType>();
        } else if (type == "bool") {
            return std::make_unique<BoolType>();
        } else if (type == "char") {
            return std::make_unique<CharType>();
        } else {
            error("Unknown type encountered " + type);
            return std::nullopt;
        };
    } else {
        return std::nullopt;
    }
}