// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "DependencySymbolInfo.h"

struct LabModule;

struct ModuleDependency {
    LabModule* module;
    DependencySymbolInfo* info;
};