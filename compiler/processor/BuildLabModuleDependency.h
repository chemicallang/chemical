// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>

class ASTFileMetaData;

struct BuildLabModuleDependency {

    /**
     * the absolute path to the directory of the module
     */
    std::string module_dir_path;

    /**
     * the file import in the build.lab that told us about this module dependency
     * this is only set in case the file is being imported in a build.lab file
     */
    ASTFileMetaData* importer_file = nullptr;

    /**
     * the scope name given by the user
     */
    chem::string_view scope_name;

    /**
     * the module name given by the user
     */
    chem::string_view mod_name;

    /**
     * constructor
     */
    BuildLabModuleDependency(
            std::string module_dir_path,
            ASTFileMetaData* importer_file,
            chem::string_view scope_name,
            chem::string_view mod_name
    ) : module_dir_path(std::move(module_dir_path)), importer_file(importer_file), scope_name(scope_name), mod_name(mod_name) {

    }

};