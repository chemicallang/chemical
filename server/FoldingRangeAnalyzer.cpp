// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "FoldingRangeAnalyzer.h"

void FoldingRangeAnalyzer::scope_begins(unsigned int position) {
    scope_start_pos_stack.push_back(scope_start_pos);
    scope_start_pos = position;
}

void FoldingRangeAnalyzer::scope_ends(unsigned int position) {

    auto start = tokens[scope_start_pos].get();
    auto end = tokens[position].get();

    ranges.push_back(FoldingRange{

            static_cast<int>(start->position.lineNumber),
            static_cast<int>(end->position.lineNumber),
            static_cast<int>(start->position.lineCharNumber),
            static_cast<int>(end->position.lineCharNumber),

            // TODO allow folding ranges of type comments
            "region"

    });

    scope_start_pos = scope_start_pos_stack.back();
    scope_start_pos_stack.pop_back();
}