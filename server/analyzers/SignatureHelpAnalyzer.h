// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "integration/common/Location.h"
#include "LibLsp/lsp/textDocument/signature_help.h"
#include "integration/cbi/model/ASTImportUnitRef.h"
#include "ast/utils/CommonVisitor.h"
#include "LspCpp/include/LibLsp/lsp/textDocument/signature_help.h"
#include "LspCpp/include/LibLsp/lsp/lsPosition.h"
#include "CaretPositionAnalyzer.h"

class ASTResult;

class SignatureHelpAnalyzer : public CommonVisitor, public CaretPositionAnalyzer {
public:

    /**
     * this allocator is disposed
     */
    ASTAllocator allocator;

    /**
     * the actual signature help collected by traversing the AST
     */
    lsSignatureHelp help;

    /**
     * constructor
     */
    SignatureHelpAnalyzer(LocationManager& loc_man, Position position);

    /**
     * this function analyzes the import unit, in which last file is the one which contains the
     * token where user asked to goto def
     * It will provide locations, where that symbol has definition
     */
    void analyze(ASTImportUnitRef& result);

    void visit(Scope *scope) final;

    void visit(FunctionCall *call) final;

};