// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <vector>
#include <std/chem_string.h>
#include "ASTFileResult.h"

/**
 * a record for existence of a dependency in build.lab or chemical.mod
 */
struct ModuleDependencyRecord {

    /**
     * the absolute path to the directory of the module
     */
    std::string module_dir_path;

    /**
     * constructor
     */
    ModuleDependencyRecord(
            std::string module_dir_path
    ) : module_dir_path(std::move(module_dir_path)) {

    }

};