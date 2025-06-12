// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "core/diag/Location.h"
#include "lsp/types.h"
#include "preprocess/visitors/RecursiveVisitor.h"
#include "InlayHintAnalyzerApi.h"
#include "core/diag/Range.h"

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
     * the range for which we are computing the inlay hints
     */
    Range range;

    /**
     * constructor
     */
    InlayHintAnalyzer(LocationManager& loc_man, const Range& range);

    bool should_compute(SourceLocation location);

    void analyze(const std::span<ASTNode*>& nodes);

    void VisitFunctionCall(FunctionCall *call);

    void VisitVarInitStmt(VarInitStatement *init);

    void VisitFunctionDecl(FunctionDeclaration *decl);

};