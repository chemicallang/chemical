// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "compiler/symres/SymbolRange.h"

struct LabModule;

class ModuleScope;

class ASTFileResult;

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
     * the path used when user imported the file
     */
    std::string import_path;

    /**
     * the absolute path determined to the file
     */
    std::string abs_path;

    /**
     * the as identifier is used with import statements to import files
     */
    std::string as_identifier;

    /**
     * this contains the parse result of the file
     */
    ASTFileResult* result = nullptr;

    /**
     * the file meta data
     */
    ASTFileMetaData(
            unsigned int file_id,
            ModuleScope* module
    ) : file_id(file_id), private_symbol_range(0, 0), module(module) {

    }

    /**
     * the file meta data
     */
    ASTFileMetaData(
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
            std::string import_path,
            std::string abs_path,
            std::string as_identifier
    ) : file_id(file_id), private_symbol_range(0, 0), module(module),
        import_path(std::move(import_path)), abs_path(std::move(abs_path)), as_identifier(std::move(as_identifier))
    {

    }

};