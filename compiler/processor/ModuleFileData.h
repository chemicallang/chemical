// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include <vector>
#include <span>
#include "ast/structures/FileScope.h"
#include "core/diag/Diagnostic.h"
#include <string>

struct ModFileSource {

    chem::string_view path;

    chem::string_view if_condition;

    bool is_negative = false;

};

typedef ModFileSource ModuleFileSource;

enum class ModFileLinkLibKind { Name, File, DefaultLib };

enum class ModFileLinkLibVisibility { Unknown };

enum class ModFileLinkLibType { Unknown };

struct ModFileLinkLib {

    chem::string_view name;

    ModFileLinkLibKind kind = ModFileLinkLibKind::Name;

    ModFileLinkLibVisibility visibility = ModFileLinkLibVisibility::Unknown;

    ModFileLinkLibType type = ModFileLinkLibType::Unknown;

    chem::string_view if_condition;

};

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
     * sources list tells us directories or files included in compilation
     */
    std::vector<ModuleFileSource> sources_list;

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

class ModuleFileDataUnit {
public:

    /**
     * allocator
     */
    ASTAllocator allocator;

    /**
     * module file data
     */
    ModuleFileData modFileData;

};