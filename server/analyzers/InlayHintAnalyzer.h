// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "integration/common/Location.h"
#include "LibLsp/lsp/textDocument/inlayHint.h"
#include "compiler/cbi/model/ASTImportUnitRef.h"
#include "preprocess/visitors/RecursiveVisitor.h"
#include "InlayHintAnalyzerApi.h"

class ASTResult;

class InlayHintAnalyzer : public RecursiveVisitor<InlayHintAnalyzer> {
public:

    /**
     * the location manager to decode locations
     */
    LocationManager& loc_man;

    /**
     * the allocator for allocating things
     */
    ASTAllocator allocator;

    /**
     * the collected hints by visiting nodes are stored here
     */
    std::vector<lsInlayHint> hints;

    /**
     * constructor
     */
    InlayHintAnalyzer(LocationManager& loc_man);

    /**
     * this function analyzes the import unit, in which last file is the one which contains the
     * token where user asked to goto def
     * It will provide locations, where that symbol has definition
     */
    std::vector<lsInlayHint> analyze(ASTImportUnitRef& result, const std::string& compiler_exe_path, const std::string& lsp_exe_path);

    void VisitFunctionCall(FunctionCall *call);

    void VisitVarInitStmt(VarInitStatement *init);

};