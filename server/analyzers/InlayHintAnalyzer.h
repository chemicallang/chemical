// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "integration/common/Location.h"
#include "LibLsp/lsp/textDocument/inlayHint.h"
#include "integration/cbi/model/ASTImportUnitRef.h"
#include "ast/utils/CommonVisitor.h"

class ASTResult;

class InlayHintAnalyzer : public CommonVisitor {
public:

    // this allocator is disposed
    ASTAllocator allocator;

    // the collected hints by visiting nodes are stored here
    std::vector<lsInlayHint> hints;

    /**
     * constructor
     */
    InlayHintAnalyzer();

    /**
     * this function analyzes the import unit, in which last file is the one which contains the
     * token where user asked to goto def
     * It will provide locations, where that symbol has definition
     */
    std::vector<lsInlayHint> analyze(ASTImportUnitRef& result, const std::string& compiler_exe_path, const std::string& lsp_exe_path);

    void visit(FunctionCall *call) override;

    void visit(VarInitStatement *init) override;

};