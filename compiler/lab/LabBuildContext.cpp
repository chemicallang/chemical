// Copyright (c) Qinetik 2024.

#include "LabBuildContext.h"
#include <unordered_map>

void LabBuildContext::add_dependencies(LabModule *mod, LabModule **dependencies, unsigned int dep_len) {
    if(!dependencies || dep_len == 0) return;
    auto ptr = *dependencies;
    unsigned i = 0;
    while (i < dep_len) {
        mod->dependencies.emplace_back(mod);
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

LabModule *LabBuildContext::add_with_type(
    LabModuleType type,
    chem::string *name,
    chem::string *path, LabModule **dependencies,
    unsigned int dep_len
) {
    modules.emplace_back(type, name->copy(), path->copy());
    auto mod = &modules.back();
    LabBuildContext::add_dependencies(mod, dependencies, dep_len);
    return mod;
}