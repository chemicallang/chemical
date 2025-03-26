// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <memory>
#include <utility>
#include <vector>
#include "ASTImportUnit.h"
#include "LexImportUnit.h"

class ASTResult;

class ASTAllocator;

class ASTImportUnit;

class GlobalInterpretScope;

struct ASTImportUnitRef {

    /**
     * this flag indicates that the unit ref was retrieved from cache entirely
     * it wasn't built a new, this helps us know that when the ast import unit
     * was built, we reported diagnostics and cleared them, since this is cached
     * we don't need to do that
     */
    const bool is_cached;

    /**
     * the absolute path to the file the import unit belongs to
     */
    std::string path;

    /**
     * stores a pointer to cached ast import unit, so if it's erased from cached
     * we still have it, only the comptime scope and allocator matters which are
     * composed in the ast import unit
     */
    std::shared_ptr<ASTImportUnit> unit;

    /**
     * all the files lex results, this is supposed to be always present and valid
     * because all files are lexed, if they can't be lexed, the diagnostics are collected and
     * stored inside the lex result present in this unit
     */
    LexImportUnit lex_unit;

    /**
     * all the files ast result, this can be empty if we didn't parse / symbol resolve
     */
    std::vector<std::shared_ptr<ASTResult>> files;

    /**
     * constructor
     */
    ASTImportUnitRef(
        bool is_cached,
        std::string abs_path,
        const std::shared_ptr<ASTImportUnit>& unit
    ) : path(std::move(abs_path)), unit(unit), lex_unit(), is_cached(is_cached) {
    }

    /**
     * constructor
     */
    ASTImportUnitRef(
        bool is_cached,
        std::string abs_path,
        const std::shared_ptr<ASTImportUnit>& unit,
        LexImportUnit lexUnit
    ) : path(std::move(abs_path)), unit(unit), lex_unit(std::move(lexUnit)), is_cached(is_cached) {

    }

    /**
     * constructor
     */
    ASTImportUnitRef(
        bool is_cached,
        std::string abs_path,
        const std::shared_ptr<ASTImportUnit>& unit,
        LexImportUnit lexUnit,
        std::vector<std::shared_ptr<ASTResult>>& ast_files
    ) : path(std::move(abs_path)), unit(unit), lex_unit(std::move(lexUnit)), is_cached(is_cached) {
        files = ast_files;
    }

};