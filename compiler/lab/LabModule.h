// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include "LabModuleType.h"

struct LabModule {

    LabModuleType type;
    chem::string name;
    chem::string path;
    std::vector<LabModule*> dependencies;

};