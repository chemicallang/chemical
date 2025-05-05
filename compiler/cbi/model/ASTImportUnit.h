// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <memory>
#include <vector>
#include "ast/base/GlobalInterpretScope.h"
#include "cst/LocationManager.h"

class ASTResult;

class LexResult;

/**
 * this ast import unit is cached in import unit cache
 * we store weak pointers, when a file changes, this weak pointer can become empty
 * then we can re get that file, and re symbol resolve the this tree and get a concrete
 * reference to import unit by locking the file pointers and getting shared pointers to them
 */
class ASTImportUnit {
public:

    /**
     * The allocator used for global interpret scope
     */
    ASTAllocator allocator;

    /**
     * the global interpret scope is used for all these files
     */
    GlobalInterpretScope comptime_scope;

    /**
     * all the files' lex results
     */
    std::vector<std::weak_ptr<LexResult>> lex_files;

    /**
     * all the files' ast results
     */
    std::vector<std::weak_ptr<ASTResult>> files;

    /**
     * this is the last file's symbol resolution diagnostics, stored for easy
     * publishing of diagnostics, a cached unit doesn't have these because it
     * doesn't store
     */
    std::vector<Diag> sym_res_diag;

    /**
     * has diagnostics been reported for this ast import unit
     */
    bool reported_diagnostics;

    /**
     * constructor
     */
    ASTImportUnit(
        std::string target_triple,
        LocationManager& loc_man,
        TypeBuilder& typeBuilder
    ) : allocator(0),
        comptime_scope(std::move(target_triple), nullptr, nullptr, allocator, typeBuilder, loc_man),
        reported_diagnostics(false)
    {

    }

};