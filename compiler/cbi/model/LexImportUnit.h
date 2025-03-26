// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include "integration/common/Diagnostic.h"
#include "preprocess/ImportGraphMaker.h"

struct LexResult;

/**
 * a import unit consists of a file and it's imported files (hierarchy)
 * an import unit consists of their lexed tokens
 *
 * a single file is included once even for multiple imports, which allows
 * us to represent them in a flat data structure like vector
 */
struct LexImportUnit {

    /**
     * the ig file representing this unit
     */
    IGFile ig_root;

    /**
     * the flat files in this import unit
     */
    std::vector<std::shared_ptr<LexResult>> files;

};