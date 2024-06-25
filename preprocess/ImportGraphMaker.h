// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <vector>
#include "integration/common/Diagnostic.h"
#include "utils/fwd/functional.h"
#include "integration/ide/model/FlatIGFile.h"
#include <memory>

class ImportPathHandler;

class Lexer;

class ImportGraphVisitor;

struct IGFile;

class CSTToken;

class ImportGraphImporter {
public:

    ImportPathHandler *handler;
    Lexer *lexer;
    ImportGraphVisitor *converter;

    /**
     * constructor
     */
    ImportGraphImporter(ImportPathHandler *handler, Lexer *lexer, ImportGraphVisitor *converter);

    /**
     * will prepare source in the lexer
     * will open the file, or prepare a stringstream in the Lexer
     * @return true if prepared
     */
    bool prepare_source(const std::string &path, std::vector<Diag> &errors);

    /**
     * will close the source
     */
    void close_source();

    /**
     * will lex the prepared source
     */
    void lex_source(const std::string &path, std::vector<Diag> &errors);

    /**
     * get the ig files from tokens
     */
    std::vector<IGFile> from_tokens(
            const std::string &path,
            IGFile *parent,
            std::vector<std::unique_ptr<CSTToken>> &tokens
    );

    /**
     * will call prepare_source, lex_source, close_source, from_tokens
     */
    virtual std::vector<IGFile> process(const std::string &path, IGFile *parent);

};

/**
 * this doesn't represent any actual file
 * its just a part of ImportGraph
 *
 * ImportGraph is constructed to analyse / process dependencies of a source file
 */
struct IGFile {

    /**
     * the pointer to the parent IGFile
     * could be null pointer, if it's the root
     */
    IGFile *parent;

    /**
     * the flat file, which is a representation of it's import statement
     */
    FlatIGFile flat_file;

    /**
     * the files imported by this file
     */
    std::vector<IGFile> files;

    /**
     * any error importing specifically this file
     */
    std::vector<Diag> errors;

    /**
     * get a list representation from this file
     */
    std::string representation();

    /**
     * flattens the file by deduplication
     * meaning only a single entry is considered for multiple files with same absolute path
     * returns de duplicated vector that contains entries from nested files and a entry for self at the last index
     */
    std::vector<FlatIGFile> flatten_by_dedupe();

};

/**
 * ImportGraph is constructed to analyse / process dependencies of a source file
 */
struct IGResult {

    /**
     * the root import graph file
     */
    IGFile root;

};

/**
 * determines the import graph
 */
IGResult determine_import_graph(ImportGraphImporter* importer, std::vector<std::unique_ptr<CSTToken>> &tokens, FlatIGFile &file);

/**
 * determines import graph from the given cst tokens
 */
IGResult
determine_import_graph(const std::string &exe_path, std::vector<std::unique_ptr<CSTToken>> &tokens, FlatIGFile &file);

/**
 * determines import graph, which is data structure
 */
IGResult determine_import_graph(const std::string &exe_path, const std::string &abs_path);

/**
 * construct's a list representation from the given IGFile
 */
void representation(IGFile &file, std::string &into, unsigned int level = 0);

/**
 * prints errors in the given file
 */
void print_errors(IGFile *file);