// Copyright (c) Qinetik 2024.

#pragma once

#include "parser/Parser.h"

void ignore_macro_lexer_fn(Parser *lexer){

}
void nested_level_macro_lexer_fn(Parser *lexer){
    lexer->lexNestedLevelMultipleStatementsTokens();
}
void eval_expression_macro_lexer_fn(Parser* lexer) {
    lexer->lexExpressionTokens(false, false);
}

const std::unordered_map<std::string_view, MacroLexerFn> MacroHandlers = {
        { "eval", eval_expression_macro_lexer_fn },
        { "sizeof", [](Parser *lexer) -> void {
            if(!lexer->lexTypeTokens()) {
                lexer->mal_type(lexer->tokens_size() - 1, "expected a type in sizeof macro");
            }
        }},
        { "target", ignore_macro_lexer_fn },
        { "target:is64bit", ignore_macro_lexer_fn },
        { "file:path", ignore_macro_lexer_fn },
        { "tr:debug:chemical", nested_level_macro_lexer_fn },
        { "tr:debug:chemical:value", eval_expression_macro_lexer_fn },
        { "tr:debug:c", nested_level_macro_lexer_fn },
        { "tr:debug:c:value", eval_expression_macro_lexer_fn },
};