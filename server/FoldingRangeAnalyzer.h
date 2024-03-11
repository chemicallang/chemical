// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once


#include "SemanticAnalyzer.h"
#include "LibLsp/lsp/textDocument/foldingRange.h"

#define DEBUG true

class FoldingRangeAnalyzer : public SemanticAnalyzer {
public:

    /**
     * constructor
     * @param tokens
     */
    FoldingRangeAnalyzer(std::vector<std::unique_ptr<LexToken>> &tokens) : SemanticAnalyzer(tokens) {

    }

    /**
     * this stack stores the start positions of the nested scopes
     * since one scope starts, we set scope_start_pos, another nested starts, we must store the previous one on the stack
     * when nested ends, we must get the last position from the scope_start_pos_stack and set it to scope_start_pos, or zero if stack is empty
     */
    std::vector<unsigned int> scope_start_pos_stack;

    /**
     * the start position (inside the tokens vector) of the current scope
     */
    unsigned int scope_start_pos = 0;

    /**
     * all the folding ranges found
     */
    std::vector<FoldingRange> ranges;

    /**
     * The function that analyzes
     */
    inline void analyze() {
        analyze_scopes();
        if (DEBUG) {
            for (const auto &range: ranges) {
                std::cout << std::to_string(range.startLine) << ':' << std::to_string(range.startCharacter) << '-'
                          << std::to_string(range.endLine) << ':' << std::to_string(range.endCharacter) << '-'
                          << range.kind << std::endl;
            }
        }
    }

    void scope_begins(unsigned int position) override;

    void scope_ends(unsigned int position) override;

};
