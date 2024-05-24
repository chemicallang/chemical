// Copyright (c) Qinetik 2024.

#include "GotoDefAnalyzer.h"
#include "cst/utils/CSTUtils.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include "lexer/model/tokens/RefToken.h"
#include <iostream>

std::string resolve_rel_path_str(const std::string &root_path, const std::string &file_path);

GotoDefAnalyzer::GotoDefAnalyzer(Position position) : position(position) {
    // do nothing
}

std::vector<Location> GotoDefAnalyzer::analyze(ImportUnit* unit) {
    auto file = unit->files[unit->files.size() - 1];
    auto token_parent = get_token_at_position(nullptr, file->tokens, position);
    if(token_parent.second == -1) {
        std::cout << "[GotoDefAnalyzer] Token at position : " << position.representation() << " not found " << std::endl;
        return {};
    }
    auto& tokens_vec = token_parent.first ? token_parent.first->tokens : file->tokens;
    auto token = tokens_vec[token_parent.second].get();
    if(token && token->is_ref()) {
        auto where = token->as_ref()->linked;
        if(where) {
            auto container = find_containing_file(unit, where);
            if(container) {
                auto end = where->end_token();
                return {
                    Location{
                        Range {
                            where->start_token()->position,
                            {end->position.line, static_cast<unsigned int>(end->position.character + where->end_token()->value.size())}
                        },
                        container->abs_path
                    }
                };
            }
        } else {
            std::cout << "[GotoDefAnalyzer] Unresolved token at position " << position.representation() << " with type " << token->type_string()  << "and with representation " << token->representation() << std::endl;
        }
    } else if(token && !token->is_ref()) {
//        if(token_parent.first && token_parent.first->type() == LexTokenType::CompImport && token->type() == LexTokenType::String) {
//            auto unquoted_str = escaped_str_token(token);
//            auto resolved = resolve_rel_path_str(file->abs_path, unquoted_str);
//            if(!resolved.empty()) {
//                return { Location { Range { Position { 0, 0}, Position { 0, 0} }, resolved } };
//            } else {
//                std::cout << "[GotoDefAnalyzer] Couldn't resolve import path " << unquoted_str << " by " << file->abs_path << std::endl;
//            }
//        } else {
//            std::cout << "[GotoDefAnalyzer] Unknown static token at position " << position.representation() << " with type " << token->type_string()  << "and with representation " << token->representation() << std::endl;
//        }
    }
    return {};
}