// Copyright (c) Qinetik 2024.

#pragma once

#include <unordered_map>
#include <string>

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