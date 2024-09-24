// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/utils/Operation.h"
#include <unordered_map>

namespace chem {
    class string;
}

class Lexer;

struct LabBuildContext;

struct LabModule;

struct LabJob;

struct ModuleArrayRef {
    LabModule** ptr;
    size_t size;
};

struct StringArrayRef {
    chem::string* ptr;
    size_t size;
};

/**
 * the function to put all symbols inside BuildContext compiler interface
 * into this unordered symbol map, the values are just function pointers
 * whereas the name's are function names
 */
void build_context_symbol_map(std::unordered_map<std::string, void*>& sym_map);

/**
 * the function to put all symbols inside for the lexer compiler interface
 */
void lexer_symbol_map(std::unordered_map<std::string, void*>& sym_map);

/**
 * the function to put all symbols inside for the source provider compiler interface
 */
void source_provider_symbol_map(std::unordered_map<std::string, void*>& sym_map);