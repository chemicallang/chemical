// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string_view>
#include <string>

#include "compiler/lab/LabJob.h"

class CompilerBinder;
class ModuleStorage;

struct BuildContextInformation {

    LabModule* root_module = nullptr;

    ModuleStorage& modStorage;

    std::vector<std::unique_ptr<LabJob>> jobs;

    CompilerBinder& binder;

    void clear();

};

/**
 * serializes the context (executables and modules) into a string
 */
std::string labBuildContext_toJsonStr(BuildContextInformation& context, bool format = false);

/**
 * gets the modules and executables from the given json
 * @return true if no error occurred, false otherwise
 */
bool labBuildContext_fromJson(BuildContextInformation& context, std::string_view jsonContent);