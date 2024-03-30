// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 07/03/2024.
//

#include "parser/Parser.h"
#include "ast/statements/Import.h"

lex_ptr<ImportStatement> Parser::parseImportStatement() {
    if (consume("import")) {
        if(token_type() == LexTokenType::String) {
            std::vector<std::string> identifiers(0);
            return std::make_unique<ImportStatement>(consume<AbstractStringToken>()->value, std::move(identifiers));
        } else if(token_type() == LexTokenType::Variable) {
            std::vector<std::string> identifiers(1);
            identifiers.push_back(consume<AbstractStringToken>()->value);
            if(!consume("from")) {
                error("expected 'from' after the path in import statement");
            }
            auto value = consume<AbstractStringToken>();
            if (value != nullptr) {
                return std::make_unique<ImportStatement>(value->value, std::move(identifiers));
            } else {
                error("expected a path after 'import' in import statement");
            }
        } else if(token_type() == LexTokenType::CharOperator && consume_op('{')) {
            std::vector<std::string> identifiers;
            do {
                if(token_type() == LexTokenType::String){
                    identifiers.push_back(consume<AbstractStringToken>()->value);
                } else {
                    break;
                }
            } while(consume_op(','));
            if(!consume("from")) {
                error("expected 'from' after the path in import statement");
            }
            identifiers.shrink_to_fit();
            auto value = consume<AbstractStringToken>();
            if (value != nullptr) {
                return std::make_unique<ImportStatement>(value->value, std::move(identifiers));
            } else {
                error("expected a path after 'import' in import statement");
            }
            if(!consume_op('}')) {
                error("expected a '}' after the identifier list in import statement");
            }
        } else {
            error("expected a path after 'import' in import statement");
        }
    }
    return std::nullopt;
}

bool Parser::parseImportStatementBool() {
    return parse_return_bool([&] () -> lex_ptr<ImportStatement> {
        return parseImportStatement();
    });
}