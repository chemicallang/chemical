// Copyright (c) Qinetik 2024.

#pragma once

#include <unordered_map>
#include <string>

/**
 * the function to put all symbols inside BuildContext compiler interface
 * into this unordered symbol map, the values are just function pointers
 * whereas the name's are function names
 */
void build_context_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);

/**
 * the function to put all symbols inside for the cst diagnoser compiler interface
 */
void cst_diagnoser_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);

/**
 * the function to put all symbols inside for the Lexer compiler interface
 */
void lexer_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);

/**
 * the function to put all symbols inside for the Lexer compiler interface
 */
void parser_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);

/**
 * the function to put all symbols inside for the SourceProvider compiler interface
 */
void source_provider_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);

/**
 * the function to put all symbols inside for the BatchAllocator compiler interface
 */
void batch_allocator_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);

/**
 * the function to put all symbols inside for the SerialStringAllocator compiler interface
 */
void serial_str_allocator_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);

/**
 * the function to put all symbols inside for the TokensVec compiler interface
 */
void ptr_vec_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);

/**
 * the function to put all symbols inside for the ASTBuilder compiler interface
 */
void ast_builder_symbol_map(std::unordered_map<std::string_view, void*>& sym_map);