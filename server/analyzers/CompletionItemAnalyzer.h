// Copyright (c) Qinetik 2024.

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
    CompletionItemAnalyzer(caret_pos_type position) : CaretPositionAnalyzer(position) {

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