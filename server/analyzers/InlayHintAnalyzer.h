// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "core/diag/Location.h"
#include "lsp/types.h"
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
    std::vector<lsp::InlayHint> hints;

    /**
     * constructor
     */
    InlayHintAnalyzer(LocationManager& loc_man);

    void VisitFunctionCall(FunctionCall *call);

    void VisitVarInitStmt(VarInitStatement *init);

};