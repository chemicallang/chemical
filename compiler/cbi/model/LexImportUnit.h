// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include "integration/common/Diagnostic.h"

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
     * the flat files in this import unit
     */
    std::vector<std::shared_ptr<LexResult>> files;

};