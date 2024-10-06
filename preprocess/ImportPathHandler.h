// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

std::string resolve_rel_parent_path_str(const std::string &root_path, const std::string &file_path);

/**
 * result of '@' replacement in path
 */
struct AtReplaceResult {
    std::string replaced;
    std::string error; // empty if no error
};

class ImportPathHandler;

using ImportPathResolverFn = AtReplaceResult(*)(ImportPathHandler& handler, const std::string& filePath, unsigned int slash);

class ImportPathHandler {
public:

    /**
     * path to the compiler's executable
     */
    std::string compiler_exe_path;

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
    std::unordered_map<std::string, ImportPathResolverFn> path_resolvers;

    /**
     * path aliases are used to basically alias a path using '@'
     * when user will import using an '@' we will
     */
    std::unordered_map<std::string, std::string> path_aliases;

    /**
     * the path to std library, we cache it once found
     */
    std::string std_lib_path;

    /**
     * constructor
     */
    ImportPathHandler(std::string compiler_exe_path);

    /**
     * get containing system headers directory for the following header
     */
    std::string headers_dir(const std::string &header);

    /**
     * replace '@' in path
     */
    AtReplaceResult replace_at_in_path(
        const std::string &filePath,
        const std::unordered_map<std::string, std::string>& aliases
    );

    /**
     * a helper method
     */
    inline AtReplaceResult replace_at_in_path(const std::string &filePath) {
        return replace_at_in_path(filePath, path_aliases);
    }

    /**
     * resolve given import path
     */
    AtReplaceResult resolve_import_path(const std::string& base_path, const std::string& import_path);


};