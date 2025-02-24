// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LibLsp/lsp/lsp_completion.h"
#include "ast/base/Visitor.h"
#include "integration/common/Position.h"
#include "CaretPositionAnalyzer.h"

class Position;

using caret_pos_type = Position;

class LexImportUnit;

class LexResult;

class ASTImportUnitRef;

class CompletionItemAnalyzer : public Visitor, public CaretPositionAnalyzer {
public:

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
    CompletionItemAnalyzer(LocationManager& manager, caret_pos_type position) : CaretPositionAnalyzer(manager, position) {

    }

    /**
     * a helper method to put simple completion items of a kind
     */
    void put(const std::string& label, lsCompletionItemKind kind);

    /**
     * put a completion item with detail and doc
     */
    void put_with_md_doc(const std::string& label, lsCompletionItemKind kind, const std::string& detail, const std::string& doc);

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
    void analyze(ASTImportUnitRef& unit);

    // Visitors

    void visit(VarInitStatement *init) final;

    void visit(AssignStatement *assign) final;

    void visit(FunctionDeclaration *functionDeclaration) final;

    void visit(EnumDeclaration *enumDeclaration) final;

    void visit(StructDefinition *structDefinition) final;

    void visit(InterfaceDefinition *interfaceDefinition) final;

    void visit(ImplDefinition *implDefinition) final;

    void visit(IfStatement *ifStatement) final;

    void visit(WhileLoop *whileLoop) final;

    void visit(DoWhileLoop *doWhileLoop) final;

    void visit(ForLoop *forLoop) final;

    void visit(SwitchStatement *statement) final;

    void visit(LambdaFunction *func) final;

    void visit(StructValue *structValue) final;

    void visit(ArrayValue *arrayVal) final;

    void visit(Scope *scope) final;

};