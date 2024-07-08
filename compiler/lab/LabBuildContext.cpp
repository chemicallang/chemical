// Copyright (c) Qinetik 2024.

#include "LabBuildContext.h"

LabModule* LabBuildContext::dir_module(chem::string* name, chem::string* path, LabModule** dependencies, unsigned int dep_len) {
    modules.emplace_back(name->copy(), path->copy());
    auto mod = &modules.back();
    auto ptr = *dependencies;
    unsigned i = 0;
    while(i < dep_len) {
        mod->dependencies.emplace_back(std::move(*mod));
        ptr++;
        i++;
    }
    return mod;
}