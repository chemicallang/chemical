// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "FoldingRangeAnalyzer.h"
#include "cst/base/CSTToken.h"
#include "ast/structures/Scope.h"
#include "cst/LocationManager.h"
#include "ast/statements/Comment.h"

void FoldingRangeAnalyzer::folding_range(const Position& start, const Position& end, bool comment) {
    ranges.push_back(FoldingRange{
            static_cast<int>(start.line),
            static_cast<int>(end.line),
            static_cast<int>(start.character),
            static_cast<int>(end.character),
            (comment ? "comment" : "region")
    });
}

void FoldingRangeAnalyzer::folding_range(SourceLocation loc, bool comment) {
    if(loc.isValid()) {
        const auto location = loc_man.getLocationPos(loc);
        folding_range(location.start, location.end, false);
    }
}

void FoldingRangeAnalyzer::visit(Scope *scope) {
    folding_range(scope->location);
}

void FoldingRangeAnalyzer::visit(Comment *comment) {
    if(comment->multiline) {
        folding_range(comment->location);
    }
}