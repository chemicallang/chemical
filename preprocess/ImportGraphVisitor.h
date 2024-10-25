// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CSTVisitor.h"

struct ImportCollected {

    // the flat file representing the import
    FlatIGFile file;

    // the range of the import
    Range range;

};

/**
 * visits the cst to collect the import statements and that's it
 */
class ImportGraphVisitor : public CSTVisitor {
public:

    /**
     * import statements collected
     */
    std::vector<ImportCollected> imports;

    void visitImport(CSTToken *importCst) final;

};