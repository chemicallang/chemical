// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "LexResult.h"
#include "ASTResult.h"
#include "std/chem_string_view.h"
#include "compiler/lab/LabModule.h"

class CachedASTUnit {
private:

    // ModuleData modifies the is_symbol_resolved flag
    friend class ModuleData;

    /**
     * is the ast unit symbol resolved
     * against current module and dependent module's files
     */
    bool is_symbol_resolved = false;

public:

    /**
     * the unit lives on this allocator
     */
    ASTAllocator allocator;

    /**
     * the pointer to unit that's cached
     */
    ASTUnit unit;

    /**
     * cached ast unit
     */
    CachedASTUnit(
            size_t allocator_batch_size,
            unsigned int file_id,
            const chem::string_view& path,
            ModuleScope* modScope
    ) : allocator(allocator_batch_size), unit(file_id, path, modScope) {

    }

    /**
     * get if this file is symbol resolved
     */
    bool get_is_symbol_resolved() {
        return is_symbol_resolved;
    }

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
    std::unordered_map<chem::string_view, std::unique_ptr<CachedASTUnit>> cachedUnits;

    /**
     * all the file units for this module, arranged in a list
     */
    std::vector<CachedASTUnit*> fileUnits;

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
     * this is set by symbol resolution and is tweaked
     * when different files change, this is used along with each files'
     * is_symbol_resoled flag, if this is true, you should ignore any file
     * flags and assume its symbol resolved, if its false, you should check
     * the file flag to know which individual file is not symbol resolved
     */
    bool is_module_symbol_resolved = false;

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
        return is_module_symbol_resolved;
    }

    /**
     * with this you can set that all files in the module have symbol resolved
     */
    void set_all_files_symbol_resolved() {
        is_module_symbol_resolved = true;
    }

    /**
     * this sets that module is not symbol resolved, and the provided
     * file is assumed to be the only one not symbol resolved inside this
     * module
     */
    void unset_all_files_symbol_resolved(CachedASTUnit& failedUnit) {
        is_module_symbol_resolved = false;
        for(const auto unit : fileUnits) {
            unit->is_symbol_resolved = true;
        }
        failedUnit.is_symbol_resolved = false;
    }

};
