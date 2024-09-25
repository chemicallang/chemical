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
 * the function to put all symbols inside for the Lexer compiler interface
 */
void lexer_symbol_map(std::unordered_map<std::string, void*>& sym_map);

/**
 * the function to put all symbols inside for the SourceProvider compiler interface
 */
void source_provider_symbol_map(std::unordered_map<std::string, void*>& sym_map);

/**
 * the function to put all symbols inside for the CSTToken compiler interface
 */
void cst_token_symbol_map(std::unordered_map<std::string, void*>& sym_map);

/**
 * the function to put all symbols inside for the TokensVec compiler interface
 */
void ptr_vec_symbol_map(std::unordered_map<std::string, void*>& sym_map);

/**
 * the function to put all symbols inside for the ASTBuilder compiler interface
 */
void ast_builder_symbol_map(std::unordered_map<std::string, void*>& sym_map);

/**
 * the function to put all symbols inside for the CSTConverter compiler interface
 */
void cst_converter_symbol_map(std::unordered_map<std::string, void*>& sym_map);