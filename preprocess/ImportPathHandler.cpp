// Copyright (c) Qinetik 2024.

#include "ImportPathhandler.h"
#include <filesystem>
#include "compiler/SelfInvocation.h"
#include "utils/PathUtils.h"

ImportPathHandler::ImportPathHandler(std::string compiler_exe_path) : compiler_exe_path(std::move(compiler_exe_path)) {

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
    if(atDirective == "system") {
        auto headerPath = filePath.substr(slash + 1);
        // Resolve the containing directory to given header
        std::string dir = headers_dir(headerPath);
        if (dir.empty()) {
            return {filePath, "couldn't resolve system headers directory for " + headerPath + " when importing"};
        }
        // Absolute path to the header
        return {(std::filesystem::path(dir) / headerPath).string(), ""};
    } else {
        auto found = path_aliases.find(atDirective);
        if(found != path_aliases.end()) {
            return { found->second + filePath.substr(slash), "" };
        } else {
            return {filePath, "unknown '@' directive " + atDirective + " in import statement"};
        }
    }
}