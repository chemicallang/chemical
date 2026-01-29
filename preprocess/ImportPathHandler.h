// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "StringViewHashEqual.h"
#include <unordered_map>
#include <vector>
#include "std/chem_string_view.h"
#include "compiler/processor/ModuleDependencyRecord.h"

struct GlobalContainer;

std::string resolve_rel_parent_path_str(const std::string &root_path, const std::string &file_path);

/**
 * result of '@' replacement in path
 */
struct AtReplaceResult {
    std::string replaced;
    std::string error; // empty if no error
};

struct ModuleIdentifier {
    chem::string_view scope_name;
    chem::string_view module_name;
};

class ASTNode;

class ImportPathHandler;

using ImportPathResolverFn = AtReplaceResult(*)(ImportPathHandler& handler, const std::string_view& filePath, unsigned int slash);

class ImportPathHandler {
public:

    /**
     * path to the compiler's executable
     */
    std::string exe_path;

    /**
     * these are the resolved places where system headers paths exist
     * when its empty, its loaded directly by invoking clang (from self)
     * then once we found them we cache them here, for faster invocation next time
     */
    std::vector<std::string> system_headers_paths = {};

    /**
     * Path resolvers are those functions that can resolve a small path to it's full absolute path
     * for example @system/std.io, where system is a path resolver, that takes the full path @system/std.io
     * including the index at where '/' occurred and returns the result of replacement
     */
    std::unordered_map<std::string, ImportPathResolverFn, StringHash, StringEqual> path_resolvers;

    /**
     * path aliases are used to basically alias a path using '@'
     * when user will import using an '@' we will
     */
    std::unordered_map<std::string, std::string, StringHash, StringEqual> path_aliases;

    /**
     * constructor
     */
    ImportPathHandler(std::string compiler_exe_path);

    /**
     * get containing system headers directory for the following header
     */
    std::string headers_dir(const std::string_view &header);

    /**
     * a module identifier is created based on import path that includes a '@' symbol in front
     */
    ModuleIdentifier get_mod_identifier_from_import_path(const std::string_view& path);

    /**
     * finds the directory path from scope and mod name
     */
    AtReplaceResult resolve_lib_dir_path(const chem::string_view& scope_name, const chem::string_view& mod_name);

    /**
     * a path can be given to get the at directive, the path has '@' in front
     */
    AtReplaceResult get_atDirective(const std::string_view& path);

    /**
     * replace '@' in path
     */
    AtReplaceResult replace_at_in_path(
            const std::string_view& filePath,
            const std::unordered_map<std::string, std::string, StringHash, StringEqual>& aliases
    );

    /**
     * a helper method
     */
    inline AtReplaceResult replace_at_in_path(const std::string_view &filePath) {
        return replace_at_in_path(filePath, path_aliases);
    }

    /**
     * resolve given import path
     */
    AtReplaceResult resolve_import_path(const std::string_view& base_path, const std::string_view& import_path);

    /**
     * in .mod or build.lab files imports on modules are done using import statements
     * this function figures out those dependencies
     */
    void figure_out_mod_dep_using_imports(
            TargetData& targetData,
            const std::string_view& base_path,
            std::vector<ModuleDependencyRecord>& buildLabModuleDependencies,
            std::vector<ASTNode*>& nodes
    );


};