// Copyright (c) Qinetik 2024.

#include "LabBuildContext.h"
#include "utils/PathUtils.h"
#include <unordered_map>

LabBuildContext::LabBuildContext(std::string lab_file, std::string user_build_dir) {
    if(user_build_dir.empty()) {
        build_dir = resolve_non_canon_parent_path(lab_file, "build");
    } else {
        build_dir = user_build_dir;
    }
}

void LabBuildContext::add_dependencies(std::vector<LabModule*>& into, LabModule **dependencies, unsigned int dep_len) {
    if(!dependencies || dep_len == 0) return;
    auto ptr = *dependencies;
    unsigned i = 0;
    while (i < dep_len) {
        into.emplace_back(ptr);
        ptr++;
        i++;
    }
}

void recursive_dedupe(LabModule* file, std::unordered_map<LabModule*, bool>& imported, std::vector<LabModule*>& flat_map) {
    for(auto nested : file->dependencies) {
        recursive_dedupe(nested, imported, flat_map);
    }
    auto found = imported.find(file);
    if(found == imported.end()) {
        imported[file] = true;
        flat_map.emplace_back(file);
    }
}

/**
 * TODO
 * 1 - avoid direct cyclic dependencies a depends on b and b depends on a
 * 2 - avoid indirect cyclic dependencies a depends on b and b depends on c and c depends on a
 */
std::vector<LabModule*> LabBuildContext::flatten_dedupe_sorted(LabModule* mod) {
    std::vector<LabModule*> modules;
    std::unordered_map<LabModule*, bool> imported;
    recursive_dedupe(mod, imported, modules);
    return modules;
}

std::vector<LabModule*> LabBuildContext::flatten_dedupe_sorted(const std::vector<LabModule*>& modules) {
    std::vector<LabModule*> new_modules;
    std::unordered_map<LabModule*, bool> imported;
    for(auto mod : modules) {
        recursive_dedupe(mod, imported, new_modules);
    }
    return new_modules;
}

LabModule *LabBuildContext::add_with_type(
    LabModuleType type,
    chem::string *name,
    chem::string *path, LabModule **dependencies,
    unsigned int dep_len
) {
    modules.emplace_back(type, name->copy(), path->copy());
    auto mod = &modules.back();
    LabBuildContext::add_dependencies(mod->dependencies, dependencies, dep_len);
    return mod;
}

LabExecutable* LabBuildContext::build_exe(
        chem::string* name,
        LabModule** dependencies,
        unsigned int dep_len
) {
    executables.emplace_back(name->copy());
    auto exe = &executables.back();
    auto build_dir_path = resolve_rel_child_path_str(build_dir, name->to_std_string() + ".dir");
    exe->build_dir.append(build_dir_path.data(), build_dir_path.size());
    auto exe_path = resolve_sibling(build_dir_path, name->to_std_string());
#ifdef _WINDOWS
    exe_path += ".exe";
#endif
    exe->abs_path.append(exe_path.data(), exe_path.size());
    LabBuildContext::add_dependencies(exe->dependencies, dependencies, dep_len);
    return exe;
}