// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "compiler/cbi/model/LexResult.h"
#include "compiler/cbi/model/ASTResult.h"
#include "std/chem_string_view.h"
#include "compiler/lab/LabModule.h"

class CachedASTUnitRef {
public:

    /**
     * the pointer to unit that's cached
     */
    ASTUnit* unit;

    /**
     * is the ast unit symbol resolved
     */
    bool is_symbol_resolved;

};

class CachedASTUnit {
public:

    /**
     * the unit lives on this allocator
     */
    ASTAllocator allocator;

    /**
     * the pointer to unit that's cached
     */
    std::unique_ptr<ASTUnit> unit;

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
    ModuleScope* modScope;

    /**
     * we store ast units for each file in this map, the unit for
     */
    std::unordered_map<chem::string_view, CachedASTUnit> cachedUnits;

    /**
     * all the file units for this module
     */
    std::vector<CachedASTUnitRef> fileUnits;

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
     * dependencies of this module, prepared when file units are prepared
     */
    std::vector<ModuleData*> dependencies;

    /**
     * constructor
     */
    ModuleData(ModuleScope* modScope) : allocator(5000/** 5kb **/), modScope(modScope) {

    }

    /**
     * check if all files inside this module unit are symbol resolved
     */
    bool all_files_symbol_resolved() {
        for(auto& unit : fileUnits) {
            if(!unit.is_symbol_resolved) {
                return false;
            }
        }
        return true;
    }

};
