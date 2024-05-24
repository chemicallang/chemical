// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "common/Location.h"
#include "LibLsp/lsp/textDocument/document_link.h"

class LexResult;

class DocumentLinksAnalyzer {
public:

    /**
     * this function analyzes the import unit, in which last file is the one which contains the
     * token where user asked to goto def
     * It will provide locations, where that symbol has definition
     */
    std::vector<lsDocumentLink> analyze(LexResult* result);

};