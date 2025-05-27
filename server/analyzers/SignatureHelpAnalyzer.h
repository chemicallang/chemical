// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "integration/common/Location.h"
#include "compiler/cbi/model/ASTImportUnitRef.h"
#include "CaretPositionAnalyzer.h"
#include "lsp/types.h"

class ASTResult;

class SignatureHelpAnalyzer : public CaretPositionAnalyzer {
public:

    /**
     * this allocator is disposed
     */
    ASTAllocator allocator;

    /**
     * the actual signature help collected by traversing the AST
     */
    lsp::SignatureHelp help;

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

};