// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LibLsp/lsp/lsp_completion.h"
#include "ast/base/Visitor.h"
#include "integration/common/Position.h"
#include "cst/base/CSTToken.h"

class Position;

using caret_pos_type = Position;

class LexImportUnit;

class LexResult;

class ASTImportUnitRef;

class CompletionItemAnalyzer : public Visitor {
public:

    /**
     * This is the position of the cursor in the document
     * The first indicates the line number (zero based)
     * The second indicates the character number (also zero based)
     */
    Position caret_position;

    /**
     * all the items that were found when analyzer completed
     */
    CompletionList list;

    /**
     * current file being analyzed
     */
    LexResult* current_file;

    /**
     * constructor
     */
    CompletionItemAnalyzer(caret_pos_type position) : caret_position(position) {}

    /**
     * get the child container that contains the caret within this compound token
     */
    CSTToken* child_container(CSTToken* compound);

    /**
     * finds the direct parent compound token
     * that contains the caret position
     */
    CSTToken* direct_parent(std::vector<CSTToken*> &tokens);

    /**
     * gets index of the token which is right before caret
     *
     * also assumes that caret is present inside these tokens
     * otherwise -1 if caret is behind all tokens, -2 if ahead of all tokens
     */
    CSTToken* token_before_caret(std::vector<CSTToken*> &tokens);

    /**
     * will return true, if given position is ahead of caret position
     * @deprecated
     */
    inline bool is_ahead(const Position& position) const {
        return position.is_ahead(caret_position);
    }

    /**
     * will return true, if given position is behind caret position
     * @deprecated
     */
    inline bool is_behind(const Position& position) const {
        return position.is_behind(caret_position);
    }

    /**
     * is equal to caret position
     * @deprecated
     */
    inline bool is_eq_caret(const Position& position) const {
        return position.is_equal(caret_position);
    }

    /**
     * will return true, if given position is ahead of caret position
     */
    bool is_ahead(CSTToken* token) const;

    /**
     * is token position equal to caret position
     */
    bool is_eq_caret(CSTToken* token) const;

    /**
     * is the cursor ahead of the given token
     */
    inline bool is_caret_ahead(CSTToken* token) const {
        return !is_ahead(token);
    }

    /**
     * is the cursor / caret behind of the given token
     */
    inline bool is_caret_behind(CSTToken* token) const {
        return is_ahead(token);
    }

    /**
     * is caret equal or behind the token's position
     */
    inline bool is_caret_eq_or_behind(CSTToken* token) const {
        return is_ahead(token) || is_eq_caret(token);
    }

    /**
     * check if caret is inside this token
     */
    bool is_caret_inside(CSTToken* token);

    /**
     * a helper method to put simple completion items of a kind
     */
    void put(const std::string& label, lsCompletionItemKind kind);

    /**
     * put a completion item with detail and doc
     */
    void put_with_md_doc(const std::string& label, lsCompletionItemKind kind, const std::string& detail, const std::string& doc);

    /**
     * chain before caret
     */
    CSTToken* chain_before_caret(std::vector<CSTToken*> &tokens);

    /**
     * would handle given access chain
     * @return true if handled
     */
    bool handle_chain_before_caret(CSTToken* token);

    /**
     * The function that analyzes tokens
     */
    CompletionList analyze(std::vector<CSTToken*> &tokens);

    /**
     * analyze an entire import unit for better support for completions across different files
     */
    CompletionList analyze(ASTImportUnitRef& unit);

    // Visitors

    void visit(VarInitStatement *init) override;

    void visit(AssignStatement *assign) override;

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(EnumDeclaration *enumDeclaration) override;

    void visit(StructDefinition *structDefinition) override;

    void visit(InterfaceDefinition *interfaceDefinition) override;

    void visit(ImplDefinition *implDefinition) override;

    void visit(IfStatement *ifStatement) override;

    void visit(WhileLoop *whileLoop) override;

    void visit(DoWhileLoop *doWhileLoop) override;

    void visit(ForLoop *forLoop) override;

    void visit(SwitchStatement *statement) override;

    void visit(LambdaFunction *func) override;

    void visit(StructValue *structValue) override;

    void visit(ArrayValue *arrayVal) override;

    void visit(Scope *scope) override;

};