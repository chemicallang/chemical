// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "integration/common/Range.h"

/**
 * in a flattened import graph, each file is represented without it's imports
 * This is a single file in a flattened import graph
 *
 * Flag IGFile contains absolute path to the file, It's supposed to be a direct representation of
 * it's import statement, for example import "file.ch", we resolve the relative path
 *
 * In the future we'll support more fields in import statement, which will be represented in this struct
 */
struct FlatIGFile {
    // the path resolved absolutely to the actual file
    std::string abs_path;
    // the path used by user in import statement
    std::string import_path;
    // as identifier
    std::string as_identifier;
    // the range at which this import file exists
    Range range;
    /**
     * this is an index into it's flat vector, which tells the compiler when to dispose this file
     * this is usually set to the file which last imported this file, so after that file has been processed, this file
     * is disposed
     */
    size_t dispose_index;
};