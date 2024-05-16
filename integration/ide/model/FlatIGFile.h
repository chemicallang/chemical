// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

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
    std::string abs_path;
};