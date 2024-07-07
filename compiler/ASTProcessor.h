// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>

#include "ast/structures/Scope.h"
#include "ASTProcessorOptions.h"
#include "preprocess/ImportGraphMaker.h"
#include "ASTDiag.h"

class Lexer;

class CSTConverter;

class SymbolResolver;

class ShrinkingVisitor;

/**
 * when a file is processed using ASTProcessor, it results in this result
 */
struct ASTImportResult {
    Scope scope;
    bool continue_processing;
    bool is_c_file;
};

/**
 * this will be called ASTProcessor
 */
class ASTProcessor {
public:

    /**
     * processor options
     */
    ASTProcessorOptions* options;

    /**
     * the import graph result that is calculated before everything
     */
    IGResult result;

    /**
     * The nodes that will be retained during compilation
     */
    std::vector<std::vector<std::unique_ptr<ASTNode>>> file_nodes;

    /**
     * The imported map, when a file is imported, it set's it's absolute path true in this map
     * to avoid re-importing files
     */
    std::unordered_map<std::string, bool> imported;

    /**
     * the symbol resolver that will resolve all the symbols
     */
    SymbolResolver* resolver;

    /**
     * it's a container of AST diagnostics
     * this is here because c file's errors are ignored because they contain unresolvable symbols
     */
    std::vector<ASTDiag> previous;

    /**
     * constructor
     */
    ASTProcessor(
            ASTProcessorOptions* options,
            SymbolResolver* resolver
    ) : options(options), resolver(resolver) {

    }

    /**
     * prepared the processing of AST
     */
    void prepare(const std::string& path);

    /**
     * get flat imports
     */
    std::vector<FlatIGFile> flat_imports();

    /**
     * lex, parse and resolve symbols in file and return Scope containing nodes
     * without performing any symbol resolution
     */
    ASTImportResult import_file_no_sym_res(const FlatIGFile& file);

    /**
     * lex, parse and resolve symbols in file and return Scope containing nodes
     * this performs symbol resolution on returned ast import result
     */
    ASTImportResult import_file(const FlatIGFile& file);

    /**
     * function that performs symbol resolution
     */
    void sym_res(Scope& scope, bool is_c_file, const std::string& abs_path);

    /**
     * called when all files are done
     */
    void end();

};