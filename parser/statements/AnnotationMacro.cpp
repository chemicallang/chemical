// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "lexer/model/tokens/MacroToken.h"
#include "ast/values/StringValue.h"
#include "ast/statements/MacroValueStatement.h"

std::optional<std::pair<std::string, std::unique_ptr<Value>>> Parser::parseMacro() {
    if (token_type() == LexTokenType::Macro) {
        auto macro = consume<MacroToken>();
        if (macro->isAnnotation) {
            return std::pair(macro->name, std::make_unique<StringValue>(""));
        } else {
            std::string content;
            while (token_type() != LexTokenType::Macro && position < tokens.size()) {
                content += as<AbstractStringToken>()->value;
                position++;
            }
            if (token_type() == LexTokenType::Macro) {
                // just going to assume this is the end token
                increment();
                // string value
                return std::pair(macro->name, std::make_unique<StringValue>(std::move(content)));
            } else {
                error("expected a macro token for ending the macro with name " + macro->name);
            }
        }
    }
    return std::nullopt;
}

bool Parser::parseMacroValueStatementBool() {
    auto value = parseMacro();
    if(!value.has_value()) return false;
    nodes.emplace_back(std::move(std::make_unique<MacroValueStatement>(std::move(value.value().first), std::move(value.value().second))));
    return true;
}