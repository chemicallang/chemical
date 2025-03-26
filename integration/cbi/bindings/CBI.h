// Copyright (c) Chemical Language Foundation 2025.

#pragma once


#include <string>
#include <span>
#include "std/chem_string_view.h"

/**
 * the function to put all symbols inside BuildContext compiler interface
 * into this unordered symbol map, the values are just function pointers
 * whereas the name's are function names
 */
void build_context_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);

/**
 * the function to put all symbols inside for the BatchAllocator compiler interface
 */
void batch_allocator_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);

/**
 * the function to put all symbols inside for the SerialStringAllocator compiler interface
 */
void serial_str_allocator_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);

/**
 * the function to put all symbols inside for the SourceProvider compiler interface
 */
void source_provider_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);

/**
 * the function to put all symbols inside for the Lexer compiler interface
 */
void lexer_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);

/**
 * the function to put all symbols inside for the Lexer compiler interface
 */
void parser_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);

/**
 * the function to put all symbols inside for the TokensVec compiler interface
 */
void ptr_vec_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);

/**
 * the function to put all symbols inside for the ASTBuilder compiler interface
 */
void ast_builder_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);

/**
 * the function to put all symbols inside for the ASTBuilder compiler interface
 */
void symbol_resolver_symbol_map(std::span<const std::pair<chem::string_view, void*>>& sym_map);