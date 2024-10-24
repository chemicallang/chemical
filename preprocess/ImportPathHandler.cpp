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
        return {filePath, "couldn't resolve system headers directory for '" + headerPath + "' when importing"};
    }
    // Absolute path to the header
    return {(std::filesystem::path(dir) / headerPath).string(), ""};
}

AtReplaceResult lib_path_resolver(
    const std::string& lib_name,
    ImportPathHandler& handler,
    const std::string& importPath,
    unsigned int slash
) {
    auto filePath = importPath.substr(slash + 1);
    std::string stdLib;
    if(!handler.std_lib_path.empty()) {
        stdLib = handler.std_lib_path;
    } else {
        const auto lib_path = "libs/" + lib_name;
#ifdef DEBUG
        const auto libsStd = resolve_sibling(handler.exe_path, lib_path);
        if(std::filesystem::exists(libsStd)) {
            // debug executable launched in a folder that contains libs/std
            stdLib = libsStd;
        } else {
            // debug executable launched in a sub folder of this project
            stdLib = resolve_sibling(resolve_parent_path(handler.exe_path), "lang/" + lib_path);
        }
#else
        stdLib = resolve_rel_parent_path_str(handler.compiler_exe_path, lib_path);
        handler.std_lib_path = stdLib;
#endif
    }
    if(stdLib.empty()) {
#ifndef DEBUG
        std::string stdLib = "libs/" + lib_name;
#endif
        return AtReplaceResult { "", "couldn't find " + lib_name + " library at path '" + stdLib + "' relative to '" + handler.exe_path + "'" };
    } else {
        auto replaced = resolve_rel_child_path_str(stdLib, filePath);
        if(replaced.empty()) {
            return AtReplaceResult { "", "couldn't find file '" + filePath + "' in " + lib_name + " library at path '" + stdLib + "'" };
        } else {
            return AtReplaceResult { replaced, "" };
        }
    }
}

AtReplaceResult std_path_resolver(ImportPathHandler& handler, const std::string& importPath, unsigned int slash) {
    return lib_path_resolver("std", handler, importPath, slash);
}

AtReplaceResult cstd_path_resolver(ImportPathHandler& handler, const std::string& importPath, unsigned int slash) {
    return lib_path_resolver("cstd", handler, importPath, slash);
}

ImportPathHandler::ImportPathHandler(std::string compiler_exe_path) : exe_path(std::move(compiler_exe_path)) {
    path_resolvers["system"] = system_path_resolver;
    path_resolvers["std"] = std_path_resolver;
    path_resolvers["cstd"] = cstd_path_resolver;
}

std::string ImportPathHandler::headers_dir(const std::string &header) {

#ifdef COMPILER_BUILD
    if(system_headers_paths.empty()) {
        system_headers_paths = ::system_headers_path(exe_path);
    }
#endif

    return ::headers_dir(system_headers_paths, header);

}

AtReplaceResult ImportPathHandler::replace_at_in_path(const std::string &filePath, const std::unordered_map<std::string, std::string>& aliases) {
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
    auto next = aliases.find(atDirective);
    if(next != aliases.end()) {
        return { next->second + filePath.substr(slash), "" };
    }
    return {filePath, "unknown '@' directive " + atDirective + " in import statement"};
}

AtReplaceResult relative_to_base(const std::string& base_path, const std::string& abs_path) {
    if(abs_path[0] == '@') {
        return { absolute_path(abs_path) };
    } else {
        auto resolved = resolve_rel_parent_path_str(base_path, abs_path);
        if (resolved.empty()) {
            return { "", "couldn't find the file to import " + abs_path + " relative to base path " + resolve_parent_path(base_path) };
        } else {
            return { resolved };
        }
    }
}

AtReplaceResult ImportPathHandler::resolve_import_path(const std::string& base_path, const std::string& import_path) {
    std::string abs_path = import_path;
    if(!abs_path.empty() && abs_path[0] == '@') {
        auto result = replace_at_in_path(abs_path);
        if(result.error.empty()) {
            abs_path = result.replaced;
        } else {
            return {"", result.error};
        }
    }
    if(!abs_path.empty()) {
        if(import_path[0] == '@') {
            abs_path = absolute_path(abs_path);
        } else {
            auto resolved = resolve_rel_parent_path_str(base_path, abs_path);
            if (resolved.empty()) {
                return { "", "couldn't find the file to import " + abs_path + " relative to base path " + resolve_parent_path(base_path) };
            } else {
                abs_path = resolved;
            }
        }
    }
    return { std::move(abs_path), "" };
}