// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LibLsp/lsp/lsp_completion.h"
#include "cst/base/CSTVisitor.h"

class Position;

using caret_pos_type = std::pair<unsigned int, unsigned int>;

class CompletionItemAnalyzer : public CSTVisitor {
public:

    /**
     * This is the position of the cursor in the document
     * The first indicates the line number (zero based)
     * The second indicates the character number (also zero based)
     */
    caret_pos_type caret_position;

    /**
     * all the items that were found when analyzer completed
     */
    std::vector<lsCompletionItem> items;

    /**
     * constructor
     */
    CompletionItemAnalyzer(caret_pos_type position) : caret_position(position) {}

    /**
     * will return true, if given position is ahead of caret position
     */
    bool is_ahead(Position& position) const;

    /**
     * check if caret is inside this token
     */
    bool is_caret_inside(CSTToken* token);

    /**
     * a helper method to put simple completion items of a kind
     */
    void put(const std::string& label, lsCompletionItemKind kind);

    /**
     * finds completion items till the given cursor position, found completion items are put on the items vector
     */
    void visit(std::vector<std::unique_ptr<CSTToken>> &tokens);

    /**
     * The function that analyzes
     */
    CompletionList analyze(std::vector<std::unique_ptr<CSTToken>> &tokens);

    // Visitors

    void visit(VarInitCST *varInit) override;

    void visit(FunctionCST *function) override;

    void visit(IfCST *ifCst) override;

    void visit(WhileCST *whileCst) override;

    void visit(DoWhileCST *doWhileCst) override;

    void visit(ForLoopCST *forLoop) override;

    void visit(SwitchCST *switchCst) override;

    void visit(StructDefCST *structDef) override;

    void visit(MultilineCommentToken *token) override;

    void visit(BodyCST *bodyCst) override;

};