// Copyright (c) Qinetik 2024.

#include "GotoDefAnalyzer.h"
#include "cst/utils/CSTUtils.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include "lexer/model/tokens/RefToken.h"

GotoDefAnalyzer::GotoDefAnalyzer(Position position) : position(position) {
    // do nothing
}

std::vector<Location> GotoDefAnalyzer::analyze(ImportUnit* unit) {
    auto file = unit->files[unit->files.size() - 1];
    auto token = get_token_at_position(file->tokens, position);
    if(token && token->is_ref()) {
        auto where = token->as_ref()->linked;
        return {
            Location{
                Range {
                    where->start_token()->position,
                    where->end_token()->position
                },
                // TODO we don't know in which file the linked token is, for now current file is being used
                file->abs_path
            }
        };
    } else {
        return {};
    }
}