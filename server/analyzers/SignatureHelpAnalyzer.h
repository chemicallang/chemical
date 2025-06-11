// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "core/diag/Location.h"
#include "server/model/ModuleData.h"
#include "CaretPositionAnalyzer.h"
#include "lsp/types.h"

class ASTResult;

class SignatureHelpAnalyzer{
public:

    /**
     * the location manager
     */
    LocationManager& loc_man;

    /**
     * the caret position given by ide
     */
    Position position;

    /**
     * the actual signature help collected by traversing the AST
     */
    lsp::SignatureHelp help;

    /**
     * constructor
     */
    SignatureHelpAnalyzer(LocationManager& loc_man, Position position);

    /**
     * the analyze function
     */
    void analyze(LabModule* module, ModuleData* modData, LexResult* lexResult, ASTUnit* unit);

};