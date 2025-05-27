// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "lsp/types.h"
#include "integration/common/Position.h"
#include "CaretPositionAnalyzer.h"
#include "preprocess/visitors/NonRecursiveVisitor.h"

class Position;

using caret_pos_type = Position;

class LexImportUnit;

class LexResult;

class ASTImportUnitRef;

class CompletionItemAnalyzer : public NonRecursiveVisitor<CompletionItemAnalyzer>, public CaretPositionAnalyzer {
public:

    /**
     * all the items that were found when analyzer completed
     */
    lsp::CompletionList list;

    /**
     * current file being analyzed
     */
    std::string_view current_file;

    /**
     * constructor
     */
    CompletionItemAnalyzer(LocationManager& manager, caret_pos_type position) : CaretPositionAnalyzer(manager, position) {

    }

//    /**
//     * a helper method to put simple completion items of a kind
//     */
//    void put(const chem::string_view& label, lsCompletionItemKind kind);
//
//    /**
//     * put a completion item with detail and doc
//     */
//    void put_with_md_doc(const chem::string_view& label, lsCompletionItemKind kind, const std::string& detail, const std::string& doc);
//
//    /**
//     * The function that analyzes tokens
//     */
//    CompletionList analyze(std::vector<CSTToken*> &tokens);

    /**
     * analyze an entire import unit for better support for completions across different files
     */
    void analyze(ASTImportUnitRef& unit);

    // Visitors

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitAssignmentStmt(AssignStatement* node);

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitEnumDecl(EnumDeclaration* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitImplDecl(ImplDefinition* node);

    void VisitIfStmt(IfStatement* node);

    void VisitWhileLoopStmt(WhileLoop* node);

    void VisitDoWhileLoopStmt(DoWhileLoop* node);

    void VisitForLoopStmt(ForLoop* node);

    void VisitSwitchStmt(SwitchStatement* node);

    void VisitLambdaFunction(LambdaFunction* value);

    void VisitStructValue(StructValue* value);

    void VisitArrayValue(ArrayValue* value);

    void VisitScope(Scope* node);

};