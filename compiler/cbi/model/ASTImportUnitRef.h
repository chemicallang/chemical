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

class LabModule;

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
     * the module this unit belongs to
     */
    LabModule* module;

    /**
     * stores a pointer to cached ast import unit, so if it's erased from cached
     * we still have it, only the comptime scope and allocator matters which are
     * composed in the ast import unit
     */
    std::shared_ptr<ASTImportUnit> unit;

    /**
     * lex result of this unit
     */
    std::shared_ptr<LexResult> lex_result;

    /**
     * ast result of this unit
     */
    std::shared_ptr<ASTResult> ast_result;

    /**
     * constructor
     */
    ASTImportUnitRef(
        bool is_cached,
        std::string abs_path,
        LabModule* module,
        const std::shared_ptr<ASTImportUnit>& unit,
        const std::shared_ptr<LexResult>& lex_result,
        const std::shared_ptr<ASTResult>& ast_result
    ) : path(std::move(abs_path)), unit(unit), module(module), lex_result(lex_result),
        ast_result(ast_result), is_cached(is_cached)
    {
    }

};