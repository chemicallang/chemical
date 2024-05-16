// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include "common/Diagnostic.h"

struct LexResult;

/**
 * a import unit consists of a file and it's imported files (hierarchy)
 * an import unit consists of their lexed tokens
 *
 * a single file is included once even for multiple imports, which allows
 * us to represent them in a flat data structure like vector
 *
 */
struct ImportUnit {

   std::vector<std::shared_ptr<LexResult>> files;

   std::vector<Diag> errors;

};