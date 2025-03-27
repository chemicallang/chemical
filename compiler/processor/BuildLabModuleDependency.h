// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <std/chem_string.h>

class ASTFileResult;

/**
 * a build lab dependency is formed by an import statement
 * which modules build.lab depends on, we have to parse every file it imports down the tree
 * when we have the tree, we figure out which imports use '@' and so we try and find the module
 * and process it which creates an object of this struct, the module_dir_path is the path
 * we have determined for that module, fileResult is only present if we have imported the file
 * in a build.lab, in that case this file may have been processed already before
 *
 */
struct BuildLabModuleDependency {

    /**
     * the absolute path to the directory of the module
     */
    std::string module_dir_path;

    /**
     * this file is the one that belongs to this module dependency and was imported from this
     * module in a build.lab, we have already processed this single file from this module
     * however we haven't parsed / compiled the entire module
     * there's one problem, this file thinks it belongs to the module we created for root build.lab
     * file, however it belongs to this module dependency that we haven't parsed / compiled, and
     * after we do that, we'll let this file know (by changing module scope pointer in it)
     */
    ASTFileResult* fileResult = nullptr;

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
    BuildLabModuleDependency(
            std::string module_dir_path,
            ASTFileResult* fileResult,
            chem::string scope_name,
            chem::string mod_name
    ) : module_dir_path(std::move(module_dir_path)), fileResult(fileResult), scope_name(std::move(scope_name)), mod_name(std::move(mod_name)) {

    }

};