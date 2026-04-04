// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "ast/base/ASTAllocator.h"
#include "compiler/lab/LabJob.h"
#include "compiler/lab/import_model/DependencySymbolInfo.h"

class CompilerBinder;
class ModuleStorage;

struct BuildContextInformation {

    LabModule* root_module = nullptr;

    ModuleStorage& modStorage;

    std::vector<std::unique_ptr<LabJob>> jobs;

    CompilerBinder& binder;

    ASTAllocator allocator;

    // Storage for deserialized data that needs to stay alive
    std::vector<std::unique_ptr<DependencySymbolInfo>> symbol_info_pool;
    std::vector<std::vector<ImportSymbol>> symbol_lists_pool;
    std::vector<std::vector<chem::string_view>> parts_lists_pool;

    // Temporary storage for mapping IDs back to modules during deserialization
    std::vector<LabModule*> id_to_module;

    void clear();

    chem::string_view pool_string(const std::string& str);

    std::string built_cbi_map_to_str();
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