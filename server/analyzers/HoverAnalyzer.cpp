// Copyright (c) Qinetik 2024.

#include "HoverAnalyzer.h"
#include "cst/utils/CSTUtils.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include "lexer/model/tokens/RefToken.h"

HoverAnalyzer::HoverAnalyzer(Position position) : position(position) {

}

std::string HoverAnalyzer::markdown_hover(ImportUnit* unit) {
    auto file = unit->files[unit->files.size() - 1];
    auto token = get_token_at_position(file->tokens, position);
    if(token && token->is_ref()) {
        auto where = token->as_ref()->linked;
        if(where) {
            switch (where->type()) {
                case LexTokenType::CompFunction:
                    value += "It's a function.";
                    break;
                case LexTokenType::CompEnumDecl:
                    value += "It's an enum. mate !";
                    break;
                case LexTokenType::CompStructDef:
                    value += "It's a struct. mate !";
                    break;
                case LexTokenType::CompInterface:
                    value += "It's an interface. buddy !";
                    break;
                case LexTokenType::CompVarInit:
                    value += "var init bud!";
                    break;
                default:
                    value += "don't know what it is";
                    break;
            }
        } else {
            value += "couldn't find the linked token !";
        }
    }
    return std::move(value);
}