// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>

struct LabModule {

    chem::string name;
    chem::string path;
    std::vector<LabModule> dependencies;

};