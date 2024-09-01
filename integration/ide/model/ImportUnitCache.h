// Copyright (c) Qinetik 2024.

#pragma once

#include "LexResult.h"
#include <unordered_map>
#include <string>
#include <memory>

/**
 * in an ide multiple files can be opened, with different import graphs, where different files
 * can sometimes import same files, these shared files shouldn't be re-parsed to improve performance
 *
 * it's also important to not over cache everything and use memory like crazy so we can allow user to edit
 * large number of files easily
 *
 * The general rule, user can open 5 - 10 files in a single session, for each of these files we'll lex
 * and cache their LexResult, in a cache with absolute paths as keys
 * the files imported by these files, will also be lexed and stored on the cache.
 * and their imports as well. recursive.
 *
 * so each file user is editing
 * we have complete lex results of it's import hierarchy, cached.
 *
 * when user changes a file's contents, we reparse that file (no other in hierarchy)
 * we only re-resolve symbols in it's importee's (dependents)
 *
 * when user closes a file, we completely get rid of it's cached hierarchy
 */
struct ImportUnitCache {

    /**
     * a cache between absolute file paths and their lex results
     * a single file's lex result without it's imports are cached so in multiple import graphs
     * same files (with same absolute path) can just make a reference to it
     */
    std::unordered_map<std::string, std::shared_ptr<LexResult>> files;

    /**
     * a cache between absolute file paths and their ast results
     * similar to how lex results are cached in ImportUnitCache
     */
    std::unordered_map<std::string, std::shared_ptr<ASTResult>> files_ast;

};