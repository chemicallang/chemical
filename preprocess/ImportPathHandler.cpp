// Copyright (c) Chemical Language Foundation 2025.

#include "ImportPathHandler.h"
#include <filesystem>
#include "compiler/SelfInvocation.h"
#include "utils/PathUtils.h"
#include "ast/statements/Import.h"
#include "ast/base/GlobalInterpretScope.h"

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
        // debug executable launched in a sub folder of this project
        return AtReplaceResult { resolve_sibling(resolve_parent_path(handler.exe_path), "lang/" + lib_path), "" };
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

AtReplaceResult ImportPathHandler::replace_at_in_path(
        const std::string_view& filePath,
        const std::unordered_map<std::string, std::string, StringHash, StringEqual>& aliases
) {
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
    auto next = aliases.find(atDirective);
    if(next != aliases.end()) {
        std::string path;
        path.append(next->second);
        path.append(filePath.substr(slash));
        return { std::move(path), "" };
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
    } else if(first_char == '/') {
        if(module_src_dir_path.empty()) {
            return { "", "cannot resolve path without the module root directory" };
        } else {
            const auto child_path = resolve_rel_child_path_str(module_src_dir_path, std::string_view(import_path.data() + 1, import_path.size() - 1));
            return { absolute_path(child_path), "" };
        }
    }
    auto resolved = resolve_rel_parent_path_str(base_path, import_path);
    if (resolved.empty()) {
        std::string errPath("couldn't find the file to import ");
        errPath.append(import_path);
        errPath.append(" relative to base path ");
        errPath.append(resolve_parent_path(base_path));
        return { "",  std::move(errPath) };
    } else {
        return { std::move(resolved), "" };
    }
}

void ImportPathHandler::figure_out_mod_dep_using_imports(
        std::vector<ModuleDependencyRecord>& buildLabModuleDependencies,
        std::vector<ASTNode*>& nodes,
        GlobalContainer* container
) {
    // some variables for processing
    std::string imp_module_dir_path;
    chem::string imp_scope_name;
    chem::string imp_mod_name;

    for(const auto stmt : nodes) {
        if(stmt->kind() == ASTNodeKind::ImportStmt) {
            const auto impStmt = stmt->as_import_stmt_unsafe();
            if(!impStmt->filePath.empty() && impStmt->filePath.ends_with(".lab")) {
                continue;
            }
            if(!impStmt->if_condition.empty()) {
                auto enabled = is_condition_enabled(container, impStmt->if_condition);
                if(enabled.has_value()) {
                    if(!enabled.value()) {
                        continue;
                    }
                } else {
                    // TODO error occurred, no such condition
                    continue;
                }
            }
            imp_module_dir_path.clear();
            imp_scope_name.clear();
            imp_mod_name.clear();
            if(impStmt->identifier.empty() && !impStmt->filePath.empty()) {
                // here we will consider this file path given a scope name and module name identifier
                auto v = impStmt->filePath.view();
                auto colInd = v.find(':');
                if(colInd != std::string_view::npos) {
                    imp_scope_name.append(chem::string_view(v.data(), colInd));
                    imp_mod_name.append(chem::string_view(v.data() + colInd + 1, v.size() - colInd));
                } else {
                    // consider the identifier a module name with scope name empty
                    imp_mod_name.append(impStmt->filePath);
                }
            } else if(!impStmt->identifier.empty()) {
                const auto idSize = impStmt->identifier.size();
                if (idSize == 1) {
                    imp_mod_name.append(impStmt->identifier[0]);
                } else if (idSize == 2) {
                    imp_scope_name.append(impStmt->identifier[0]);
                    imp_mod_name.append(impStmt->identifier[1]);
                } else {
                    // TODO handle the error
                }
            }
            if(!imp_mod_name.empty()) {
                auto result = resolve_lib_dir_path(imp_scope_name.to_chem_view(), imp_mod_name.to_chem_view());
                if(result.error.empty()) {
                    imp_module_dir_path.append(result.replaced);
                } else {
                    // TODO handle the error
                }
            }
            if(!impStmt->filePath.empty() && !impStmt->identifier.empty()) {
                // TODO: explicit import for a directory with scope name and module name
                imp_module_dir_path.append(impStmt->filePath.data(), impStmt->filePath.size());
            }
            buildLabModuleDependencies.emplace_back(std::move(imp_module_dir_path), std::move(imp_scope_name), std::move(imp_mod_name));
        }
    }
}