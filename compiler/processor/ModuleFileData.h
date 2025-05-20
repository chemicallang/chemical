// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include <vector>
#include <span>
#include "ast/structures/FileScope.h"
#include "integration/common/Diagnostic.h"
#include <string>

class ModuleFileData {
public:

    /**
     * the scope name in the .mod file's module declaration
     */
    chem::string_view scope_name;

    /**
     * the module name in the .mod file's module declaration
     */
    chem::string_view module_name;

    /**
     * the scope is used to contain the file
     */
    FileScope scope;

    /**
     * interface declarations in chemical.mod file allowing user to import
     * compiler interfaces easily
     */
    std::vector<chem::string_view> compiler_interfaces;

    /**
     * this is not empty if a ready error occurred
     */
    std::string read_error;

    /**
     * diagnostics for the .mod file
     */
    std::vector<Diag> diagnostics;

    /**
     * the file id is not given and module scope is also null because file
     * doesn't belong to a module
     */
    ModuleFileData(
            unsigned int fileId,
            const chem::string_view& filePath
    ) : scope(fileId, filePath, nullptr) {

    }

};