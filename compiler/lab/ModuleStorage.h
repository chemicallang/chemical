// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include "LabModule.h"

class ModuleStorage {
private:

    /**
     * all the modules created during build process are stored in this vector
     */
    std::vector<std::unique_ptr<LabModule>> modules;

    /**
     * this allows to quickly retrieve a module by it's scope:module_name
     * the format is pretty specific (with a colon in between) -> scope:module
     */
    std::unordered_map<std::string, LabModule*> indexes;

public:

    /**
     * get the modules
     */
    inline const std::vector<std::unique_ptr<LabModule>>& get_modules() {
        return modules;
    }

    /**
     * would just index this module
     */
    void index_module(LabModule* module);

    /**
     * insert a module into the modules
     */
    void insert_module(std::unique_ptr<LabModule> modulePtr) {
        const auto module = modulePtr.get();
        modules.emplace_back(std::move(modulePtr));
        index_module(module);
    }

    /**
     * insert module ptr dangerously
     */
    inline void insert_module_ptr_dangerous(LabModule* module) {
        insert_module(std::unique_ptr<LabModule>(module));
    }

    /**
     * find a module with scope:name format
     */
    inline LabModule* find_module(const std::string& scope_name_format) {
        auto found = indexes.find(scope_name_format);
        return found == indexes.end() ? nullptr : found->second;
    }

    /**
     * this gives the total modules
     */
    inline size_t modules_size() {
        return modules.size();
    }

    /**
     * this would clear any modules or indexes
     * as if this storage started anew
     */
    void clear() {
        modules.clear();
        indexes.clear();
    }

};