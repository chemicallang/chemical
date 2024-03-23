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

lex_ptr<BaseType> Parser::parseType() {
    if (token_type() == LexTokenType::Type) {
        auto type = std::move(consume<TypeToken>()->value);
        auto type_fn = primitive_type_map.find(type);
        if(type_fn != primitive_type_map.end()) {
            return std::unique_ptr<BaseType>(type_fn->second(this));
        } else {
            return std::make_unique<ReferencedType>(type);
        }
    } else {
        return std::nullopt;
    }
}