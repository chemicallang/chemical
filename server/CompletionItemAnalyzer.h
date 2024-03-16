// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#ifdef LSP_BUILD
#include "SemanticAnalyzer.h"
#include "LibLsp/lsp/lsp_completion.h"

#define DEBUG false

using cursor_position = std::pair<unsigned int, unsigned int>;

class CompletionItemAnalyzer : public SemanticAnalyzer {
public:

    /**
     * This is the position of the cursor in the document
     * The first indicates the line number (zero based)
     * The second indicates the character number (also zero based)
     */
    cursor_position position;

    // constructor
    CompletionItemAnalyzer(std::vector<std::unique_ptr<LexToken>> &tokens, cursor_position position)
            : SemanticAnalyzer(tokens), position(position) {}

    /**
     * all the items that were found when analyzer completed
     */
    std::vector<lsCompletionItem> items;

    /**
     * finds completion items till the given cursor position, found completion items are put on the items vector
     */
    void find_completion_items();

    /**
     * The function that analyzes
     */
    inline CompletionList analyze() {
        find_completion_items();
        if(DEBUG) {
            for(const auto & item : items) {
                std::cout << item.label << std::endl;
            }
        }
        return CompletionList{false, std::move(items)};
    }

};
#endif