// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once


#include "LibLsp/lsp/textDocument/foldingRange.h"
#include "SemanticAnalyzer.h"

#define DEBUG false

class FoldingRangeAnalyzer : public SemanticAnalyzer {
public:

    /**
     * constructor
     * @param tokens
     */
    FoldingRangeAnalyzer(std::vector<std::unique_ptr<LexToken>> &tokens) : SemanticAnalyzer(tokens) {

    }

    /**
     * all the folding ranges found
     */
    std::vector<FoldingRange> ranges;

    /**
     * analyzes scopes to create folding ranges
     */
    void analyze_scopes();

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

};
