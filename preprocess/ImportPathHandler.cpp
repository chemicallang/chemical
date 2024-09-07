// Copyright (c) Qinetik 2024.

#include "ImportPathhandler.h"
#include <filesystem>
#include "compiler/SelfInvocation.h"
#include "utils/PathUtils.h"

AtReplaceResult system_path_resolver(ImportPathHandler& handler, const std::string& filePath, unsigned int slash) {
    auto headerPath = filePath.substr(slash + 1);
    // Resolve the containing directory to given header
    std::string dir = handler.headers_dir(headerPath);
    if (dir.empty()) {
        return {filePath, "couldn't resolve system headers directory for " + headerPath + " when importing"};
    }
    // Absolute path to the header
    return {(std::filesystem::path(dir) / headerPath).string(), ""};
}

AtReplaceResult std_path_resolver(ImportPathHandler& handler, const std::string& importPath, unsigned int slash) {
    auto filePath = importPath.substr(slash + 1);
    std::string stdLib;
    if(handler.std_lib_path.empty()) {
        stdLib = handler.std_lib_path;
    } else {
#ifdef DEBUG
        // we store the
        stdLib = "lang/std";
#else
        stdLib = resolve_rel_child_path_str(handler.compiler_exe_path, "libs/std");
        handler.std_lib_path = stdLib;
#endif
    }
    if(stdLib.empty()) {
#ifndef DEBUG
        std::string stdLib = "libs/std";
#endif
        return AtReplaceResult { "", "couldn't find std library at path '" + stdLib + "' relative to '" + handler.compiler_exe_path + "'" };
    } else {
        auto replaced = resolve_rel_child_path_str(stdLib, filePath);
        if(replaced.empty()) {
            return AtReplaceResult { "", "couldn't find file '" + filePath + "' in std library at path '" + stdLib + "'" };
        } else {
            return AtReplaceResult { replaced, "" };
        }
    }
}

ImportPathHandler::ImportPathHandler(std::string compiler_exe_path) : compiler_exe_path(std::move(compiler_exe_path)) {
    path_resolvers["system"] = system_path_resolver;
    path_resolvers["std"] = std_path_resolver;
}

std::string ImportPathHandler::headers_dir(const std::string &header) {

    if(system_headers_paths.empty()) {
        system_headers_paths = std::move(::system_headers_path(compiler_exe_path));
    }

    return ::headers_dir(system_headers_paths, header);

}

AtReplaceResult ImportPathHandler::replace_at_in_path(const std::string& filePath) {
    if(filePath[0] != '@') return {filePath, ""};
    auto slash = filePath.find('/');
    if(slash == -1) {
        return {filePath, "couldn't find '/' in the file path, which must be present if using '@' directive" };
    }
    auto atDirective = filePath.substr(1, slash - 1);
    auto found = path_resolvers.find(atDirective);
    if(found != path_resolvers.end()) {
        return found->second(*this, filePath, slash);
    }
    auto next = path_aliases.find(atDirective);
    if(next != path_aliases.end()) {
        return { next->second + filePath.substr(slash), "" };
    }
    return {filePath, "unknown '@' directive " + atDirective + " in import statement"};
}