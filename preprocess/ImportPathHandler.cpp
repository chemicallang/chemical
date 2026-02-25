// Copyright (c) Chemical Language Foundation 2025.

#include "ImportPathHandler.h"
#include <filesystem>
#include "compiler/SelfInvocation.h"
#include "utils/PathUtils.h"
#include "ast/statements/Import.h"
#include "ast/base/GlobalInterpretScope.h"
#include "compiler/lab/TargetConditionAPI.h"

AtReplaceResult system_path_resolver(ImportPathHandler& handler, const std::string_view& filePath, unsigned int slash) {
    auto headerPath = filePath.substr(slash + 1);
    // Resolve the containing directory to given header
    std::string dir = handler.headers_dir(headerPath);
    if (dir.empty()) {
        return { std::string(filePath), "couldn't resolve system headers directory for '" + std::string(headerPath) + "' when importing"};
    }
    // Absolute path to the header
    return { (std::filesystem::path(dir) / headerPath).string(), ""};
}

AtReplaceResult lib_path_resolver(
        const std::string& lib_name,
        ImportPathHandler& handler
) {
    std::string libPath;
    const auto lib_path = "libs/" + lib_name;
#ifdef DEBUG
    const auto libsStd = resolve_sibling(handler.exe_path, lib_path);
    if (std::filesystem::exists(libsStd)) {
        // debug executable launched in a folder that contains libs/std
        libPath = libsStd;
    } else {
        // debug executable launched with working dir = project source dir
        libPath = resolve_rel_child_path_str(PROJECT_SOURCE_DIR, "lang/" + lib_path);
    }
#else
    libPath = resolve_rel_parent_path_str(handler.exe_path, lib_path);
#endif
    if(libPath.empty()) {
        return AtReplaceResult { "", "couldn't find " + lib_name + " library at path '" + libPath + "' relative to '" + handler.exe_path + "'" };
    } else {
        return AtReplaceResult { std::move(libPath), "" };
    }
}

AtReplaceResult lib_path_resolver(
        const std::string_view& lib_name,
        ImportPathHandler& handler
) {
    std::string lib_path("libs/");
    lib_path.append(lib_name);
#ifdef DEBUG
    const auto libsStd = resolve_sibling(handler.exe_path, lib_path);
    if (std::filesystem::exists(libsStd)) {
        // debug executable launched in a folder that contains libs/std
        return AtReplaceResult { std::move(libsStd), "" };
    } else {
        // debug executable launched with working dir = project source dir
        return AtReplaceResult { resolve_rel_child_path_str(PROJECT_SOURCE_DIR, "lang/" + lib_path), "" };
    }
#else
    auto libPath = resolve_rel_parent_path_str(handler.exe_path, lib_path);
    if(libPath.empty()) {
        std::string errStr("couldn't find '");
        errStr.append(lib_name);
        errStr.append("' library at path '");
        errStr.append(libPath);
        errStr.append("' relative to '");
        errStr.append(handler.exe_path);
        errStr.append(1, '\'');
        return AtReplaceResult { "", std::move(errStr) };
    } else {
        return AtReplaceResult { std::move(libPath), "" };
    }
#endif
}

ModuleIdentifier ImportPathHandler::get_mod_identifier_from_import_path(const std::string_view& filePath) {
    if(filePath[0] != '@') return { "", "" };
    auto slash = filePath.find('/');
    std::string_view directive_view;
    if(slash == std::string::npos) {
        directive_view = std::string_view(filePath.data() + 1);
    } else {
        directive_view = std::string_view(filePath.data() + 1, slash - 1);
    }
    auto found = directive_view.find(':');
    if(found == std::string_view::npos) {
        return { "", chem::string_view(directive_view) };
    }
    return { chem::string_view(directive_view.data(), found), chem::string_view(directive_view.data() + found, directive_view.size() - found) };
}

std::string ImportPathHandler::resolve_native_lib(const chem::string_view& mod_name) {
    auto lib_path = std::string("libs/");
    lib_path.append(mod_name.view());
#ifdef DEBUG
    const auto libsStd = resolve_sibling(exe_path, lib_path);
    if (std::filesystem::exists(libsStd)) {
        // debug executable launched in a folder that contains libs/std
        return libsStd;
    } else {
        // debug executable launched with working dir = project source dir
        return resolve_rel_child_path_str(PROJECT_SOURCE_DIR, "lang/" + lib_path);
    }
#else
    return resolve_sibling(exe_path, lib_path);
#endif
}

AtReplaceResult ImportPathHandler::resolve_lib_dir_path(const chem::string_view& scope_name, const chem::string_view& mod_name) {
    // TODO currently we don't handle the scope name
    return lib_path_resolver(mod_name.str(), *this);
}

AtReplaceResult lib_src_path_resolver(
        const std::string& lib_name,
        ImportPathHandler& handler
) {
    std::string libPath;
    const auto lib_path = "libs/" + lib_name + "/src";
#ifdef DEBUG
    const auto libsStd = resolve_sibling(handler.exe_path, lib_path);
    if (std::filesystem::exists(libsStd)) {
        // debug executable launched in a folder that contains libs/std
        libPath = libsStd;
    } else {
        // debug executable launched in a sub folder of this project
        libPath = resolve_sibling(resolve_parent_path(handler.exe_path), "lang/" + lib_path);
    }
#else
    libPath = resolve_rel_parent_path_str(handler.exe_path, lib_path);
#endif
    if(libPath.empty()) {
        return AtReplaceResult { "", "couldn't find " + lib_name + " library at path '" + libPath + "' relative to '" + handler.exe_path + "'" };
    } else {
        return AtReplaceResult { std::move(libPath), "" };
    }
}

AtReplaceResult lib_path_replacer(
    const std::string& lib_name,
    ImportPathHandler& handler,
    const std::string& importPath,
    unsigned int slash
) {
    auto result = lib_path_resolver(lib_name, handler);
    if(result.error.empty() && slash + 1 < importPath.size()) {
        auto filePath = importPath.substr(slash + 1);
        auto replaced = resolve_rel_child_path_str(result.replaced, filePath);
        if(replaced.empty()) {
            return AtReplaceResult { "", "couldn't find file '" + filePath + "' in " + lib_name + " library at path '" + result.replaced + "'" };
        } else {
            return AtReplaceResult { replaced, "" };
        }
    } else {
        return result;
    }
}

AtReplaceResult lib_path_replacer(
        const std::string_view& lib_name,
        ImportPathHandler& handler,
        const std::string_view& importPath,
        unsigned int slash
) {
    auto result = lib_path_resolver(lib_name, handler);
    if(result.error.empty() && slash + 1 < importPath.size()) {
        auto filePath = importPath.substr(slash + 1);
        auto replaced = resolve_rel_child_path_str(result.replaced, filePath);
        if(replaced.empty()) {
            std::string errStr("couldn't find file '");
            errStr.append(filePath);
            errStr.append("' in ");
            errStr.append(lib_name);
            errStr.append(" library at path '");
            errStr.append(result.replaced);
            errStr.append(1, '\'');
            return AtReplaceResult { "", std::move(errStr) };
        } else {
            return AtReplaceResult { replaced, "" };
        }
    } else {
        return result;
    }
}

ImportPathHandler::ImportPathHandler(std::string compiler_exe_path) : exe_path(std::move(compiler_exe_path)) {
    path_resolvers["system"] = system_path_resolver;
}

std::string ImportPathHandler::headers_dir(const std::string_view &header) {

#ifdef COMPILER_BUILD
    if(system_headers_paths.empty()) {
        system_headers_paths = ::system_headers_path(exe_path);
    }
#endif

    return ::headers_dir(system_headers_paths, header);

}

AtReplaceResult ImportPathHandler::get_atDirective(const std::string_view& filePath) {
    if(filePath[0] != '@') return { std::string(filePath), "" };
    auto slash = filePath.find('/');
    if(slash == std::string::npos) {
        return { std::string(filePath), "" };
    }
    return { std::string(filePath.substr(1, slash - 1)), "" };
}

AtReplaceResult ImportPathHandler::replace_at_in_path(const std::string_view& filePath) {
    if(filePath[0] != '@') return {std::string(filePath), ""};
    auto slash = filePath.find('/');
    if(slash == std::string::npos) {
        slash = filePath.size();
    }
    auto atDirective = filePath.substr(1, slash - 1);
    auto found = path_resolvers.find(atDirective);
    if(found != path_resolvers.end()) {
        return found->second(*this, filePath, slash);
    }
    auto resolver = lib_path_replacer(atDirective, *this, filePath, slash);
    if(resolver.error.empty()) {
        return resolver;
    }
    return { std::string(filePath), "unknown '@' directive " + std::string(atDirective) + " in import statement"};
}

AtReplaceResult ImportPathHandler::resolve_import_path(const std::string_view& base_path, const std::string_view& import_path) {
    const auto first_char = import_path[0];
    if(first_char == '@') {
        auto result = replace_at_in_path(import_path);
        if(result.error.empty()) {
            return { absolute_path(result.replaced), "" };
        } else {
            return { "", result.error };
        }
    }
    return { absolute_path(resolve_sibling(base_path, import_path)), "" };
}

ImportedModuleDepResult ImportPathHandler::resolve_mod_dep_import(
    ImportStatement* stmt,
    TargetData& targetData,
    const std::string_view& base_path
) {

    // skip remote imports
    if(!stmt->isLocalModuleImport()) {
        return ImportedModuleDepResult { };
    }

    // check are we even supposed to include this import
    if(stmt->if_condition != nullptr) {
        auto enabled = resolve_target_condition(targetData, stmt->if_condition);
        if(enabled.has_value()) {
            if(!enabled.value()) {
                return ImportedModuleDepResult { };
            }
        } else {
            return ImportedModuleDepResult { .error_message = "couldn't evaluate the if condition" };
        }
    }

    // handle native libs: import std
    auto& sourcePath = stmt->getSourcePath();
    if(stmt->isNativeLibImport()) {
        return ImportedModuleDepResult { .directory_path = resolve_native_lib(sourcePath) };
    }

    // check relative imports to directory
    if(sourcePath.starts_with("./") || sourcePath.starts_with("../")) {
        // user has given relative path to a directory / file
        return ImportedModuleDepResult { .directory_path = resolve_sibling(base_path, sourcePath.view()) };
    }

    return ImportedModuleDepResult { .error_message = "couldn't determine import for module" };

}