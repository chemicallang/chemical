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

    inline static std::string build_format(const chem::string_view& scope_name, const chem::string_view& mod_name) {
        return LabModule::format(scope_name, mod_name, ':');
    }

    /**
     * get the modules
     */
    inline const std::vector<std::unique_ptr<LabModule>>& get_modules() {
        return modules;
    }

    /**
     * would just index this module
     */
    void index_module(LabModule* module) {
        indexes[build_format(module->scope_name.to_chem_view(), module->name.to_chem_view())] = module;
    }

    /**
     * would index module and its dependencies
     */
    void index_modules(std::vector<LabModule*>& mods) {
        for(const auto mod : mods) {
            index_module(mod);
        }
    }

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
     * this will return a cached pointer, if the module already exists
     */
    inline LabModule* find_module(const chem::string_view& scope_name, const chem::string_view& mod_name) {
        return find_module(build_format(scope_name, mod_name));
    }

    /**
     * this gives the total modules
     */
    inline size_t modules_size() {
        return modules.size();
    }

    /**
     * only indexes are cleared
     */
    void clear_indexes() {
        indexes.clear();
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