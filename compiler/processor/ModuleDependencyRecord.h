// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <vector>
#include <std/chem_string.h>
#include "ASTFileResult.h"

/**
 * a build lab dependency is formed by an import statement
 * which modules build.lab depends on, we have to parse every file it imports down the tree
 * when we have the tree, we figure out which imports use '@' and so we try and find the module
 * and process it which creates an object of this struct, the module_dir_path is the path
 * we have determined for that module, fileResult is only present if we have imported the file
 * in a build.lab, in that case this file may have been processed already before
 *
 */
struct ModuleDependencyRecord {

    /**
     * the absolute path to the directory of the module
     */
    std::string module_dir_path;

    /**
     * the scope name given by the user
     */
    chem::string scope_name;

    /**
     * the module name given by the user
     */
    chem::string mod_name;

    /**
     * constructor
     */
    ModuleDependencyRecord(
            std::string module_dir_path,
            chem::string scope_name,
            chem::string mod_name
    ) : module_dir_path(std::move(module_dir_path)), scope_name(std::move(scope_name)), mod_name(std::move(mod_name)) {

    }

};