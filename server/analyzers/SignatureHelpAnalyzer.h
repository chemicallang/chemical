// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "integration/common/Location.h"
#include "LibLsp/lsp/textDocument/signature_help.h"
#include "integration/cbi/model/ASTImportUnitRef.h"
#include "ast/utils/CommonVisitor.h"
#include "LspCpp/include/LibLsp/lsp/textDocument/signature_help.h"
#include "LspCpp/include/LibLsp/lsp/lsPosition.h"

class ASTResult;

class SignatureHelpAnalyzer : public CommonVisitor {
public:

    // this allocator is disposed
    ASTAllocator allocator;

    /*
     * the actual signature help collected by traversing the AST
     */
    lsSignatureHelp help;

    /**
     * the position of the cursor
     */
    Position cursor_pos;

    /**
     * constructor
     */
    SignatureHelpAnalyzer();

    /**
     * this function analyzes the import unit, in which last file is the one which contains the
     * token where user asked to goto def
     * It will provide locations, where that symbol has definition
     */
    void analyze(ASTImportUnitRef& result, const lsPosition& position);

    void visit(Scope *scope) override;

    void visit(FunctionCall *call) override;

};