// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "compiler/symres/SymbolRange.h"

struct LabModule;

class ModuleScope;

struct ASTFileResult;

class ImportStatement;

struct ASTFileMetaData {

    /**
     * the id of the file
     */
    unsigned int file_id;

    /**
     * the scope index is the index of scope where symbols in symbol resolver exist
     */
    SymbolRange private_symbol_range;

    /**
     * the module this file belongs to
     */
    ModuleScope* module;

    /**
     * the absolute path determined to the file
     */
    std::string abs_path;

    /**
     * the import statement used to import this file
     * only set when there exists a import statement, this will be nullptr for most files
     */
    ImportStatement* stmt = nullptr;

    /**
     * this contains the parse result of the file
     */
    ASTFileResult* result = nullptr;

    /**
     * the file meta data
     */
    constexpr ASTFileMetaData(
            unsigned int file_id,
            ModuleScope* module
    ) : file_id(file_id), private_symbol_range(0, 0), module(module) {

    }

    /**
     * the file meta data
     */
    constexpr ASTFileMetaData(
            unsigned int file_id,
            ModuleScope* module,
            std::string abs_path
    ) : file_id(file_id), private_symbol_range(0, 0), module(module), abs_path(std::move(abs_path)) {

    }

    /**
     * constructor
     */
    ASTFileMetaData(
            unsigned int file_id,
            ModuleScope* module,
            std::string abs_path,
            ImportStatement* stmt
    ) : file_id(file_id), private_symbol_range(0, 0), module(module),
        abs_path(std::move(abs_path)), stmt(stmt) {

    }

};