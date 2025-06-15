// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "LexResult.h"
#include "ASTResult.h"
#include "std/chem_string_view.h"
#include "compiler/lab/LabModule.h"

class CachedASTUnit {
public:

    /**
     * the unit lives on this allocator
     */
    ASTAllocator allocator;

    /**
     * the pointer to unit that's cached
     */
    ASTUnit unit;

};


class ModuleData {
public:

    /**
     * the module's module level allocator
     */
    ASTAllocator allocator;

    /**
     * this is created once
     */
    ModuleScope modScope;

    /**
     * we store ast units for each file in this map, the unit for
     */
    std::unordered_map<chem::string_view, std::unique_ptr<CachedASTUnit>> cachedUnits;

    /**
     * all the file units for this module, arranged in a list
     */
    std::vector<CachedASTUnit*> fileUnits;

    /**
     * the files that have changed are stored inside
     */
    std::unordered_set<CachedASTUnit*> dirtyFiles;

    /**
     * used for synchronization of parsing and symbol resolution of this module
     */
    std::mutex module_mutex;

    /**
     * this is set to true, by one thread to indicate that we've prepared
     * the fileUnits vector in this ModuleData
     */
    bool prepared_file_units = false;

    /**
     * a module to be symbol resolved, it must not have dirty files
     * and must be symbol resolved completely at least once
     */
    bool symbol_resolved_once = false;

    /**
     * dependencies of this module, prepared when file units are prepared
     */
    std::vector<ModuleData*> dependencies;

    /**
     * constructor
     */
    ModuleData(
            const chem::string_view& scope_name,
            const chem::string_view& mod_name,
            LabModule* container
    ) : allocator(5000/** 5kb **/), modScope(scope_name, mod_name, container) {

    }

    /**
     * get the module
     */
    inline LabModule* getModule() {
        return modScope.container;
    }

    /**
     * check if all files inside this module unit are symbol resolved
     */
    inline bool completely_symbol_resolved() {
        return symbol_resolved_once && dirtyFiles.empty();
    }

    /**
     * check if the given file is dirty (not symbol resolved / changed)
     */
    inline bool is_dirty(CachedASTUnit* unit) {
        return dirtyFiles.contains(unit);
    }

    /**
     * with this you can set that all files in the module have symbol resolved
     */
    inline void set_completely_symbol_resolved() {
        symbol_resolved_once = true;
        dirtyFiles.clear();
    }

    /**
     * this sets that module is not symbol resolved, and the provided
     * file is assumed to be the only one not symbol resolved inside this
     * module
     */
    void make_single_file_dirty(CachedASTUnit* failedUnit) {
        dirtyFiles.clear();
        dirtyFiles.insert(failedUnit);
    }

};
