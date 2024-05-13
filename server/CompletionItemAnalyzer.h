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

    void visitVarInit(CompoundCSTToken *varInit) override;

    void visitFunction(CompoundCSTToken *function) override;

    void visitIf(CompoundCSTToken *ifCst) override;

    void visitWhile(CompoundCSTToken *whileCst) override;

    void visitDoWhile(CompoundCSTToken *doWhileCst) override;

    void visitForLoop(CompoundCSTToken *forLoop) override;

    void visitSwitch(CompoundCSTToken *switchCst) override;

    void visitStructDef(CompoundCSTToken *structDef) override;

    void visit(MultilineCommentToken *token) override;

    void visitBody(CompoundCSTToken *bodyCst) override;

};