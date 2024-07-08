// Copyright (c) Qinetik 2024.

#include "LabBuildContext.h"

void LabBuildContext::add_dependencies(LabModule *mod, LabModule **dependencies, unsigned int dep_len) {
    auto ptr = *dependencies;
    unsigned i = 0;
    while (i < dep_len) {
        mod->dependencies.emplace_back(mod);
        ptr++;
        i++;
    }
}

LabModule *LabBuildContext::add_with_type(
    LabModuleType type,
    chem::string *name,
    chem::string *path, LabModule **dependencies,
    unsigned int dep_len
) {
    modules.emplace_back(type, name->copy(), path->copy());
    const auto mod = &modules.back();
    LabBuildContext::add_dependencies(mod, dependencies, dep_len);
    return mod;
}