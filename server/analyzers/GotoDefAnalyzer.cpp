// Copyright (c) Qinetik 2024.

#include "GotoDefAnalyzer.h"
#include "cst/utils/CSTUtils.h"
#include "integration/cbi/model/LexImportUnit.h"
#include "integration/cbi/model/LexResult.h"
#include <iostream>
#include "utils/PathUtils.h"
#include "ast/base/ASTNode.h"
#include "cst/LocationManager.h"

GotoDefAnalyzer::GotoDefAnalyzer(
    LocationManager& manager,
    Position position
) : manager(manager), position(position) {
    // do nothing
}

std::vector<Location> GotoDefAnalyzer::analyze(LexImportUnit* unit) {
    auto file = unit->files[unit->files.size() - 1];
    auto token_parent = get_token_at_position(nullptr, file->unit.tokens, position);
    if(token_parent.second == -1) {
        std::cout << "[GotoDefAnalyzer] Token at position : " << position.representation() << " not found " << std::endl;
        return {};
    }
    auto& tokens_vec = token_parent.first ? token_parent.first->tokens : file->unit.tokens;
    auto token = tokens_vec[token_parent.second];
    if(token && token->is_ref() && token->any) {
        const auto ref_linked = token->any->get_ref_linked_node();
        if(ref_linked) {
            const auto encoded = ref_linked->encoded_location();
            const auto location = manager.getLocationPos(encoded);
            const auto filePath = manager.getPathForFileId(location.fileId);
            return {
                    Location{
                            Range {
                                    location.start,
                                    location.end
                            },
                            std::string(filePath)
                    }
            };
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