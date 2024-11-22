// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <vector>
#include "integration/common/Diagnostic.h"
#include "utils/fwd/functional.h"
#include "integration/cbi/model/FlatIGFile.h"
#include <memory>
#include "ImportPathHandler.h"

class Parser;

class ImportGraphVisitor;

struct IGFile;

class CSTToken;

class LocationManager;

class ImportGraphImporter {
public:

    ImportPathHandler *handler;
    Parser *lexer;
    ImportGraphVisitor *converter;

    /**
     * constructor
     */
    ImportGraphImporter(ImportPathHandler *handler, Parser *lexer, ImportGraphVisitor *converter);

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
            std::vector<CSTToken*> &tokens
    );

    /**
     * will call prepare_source, lex_source, close_source, from_tokens
     */
    virtual std::vector<IGFile> process(const std::string &path, const Range& range, IGFile *parent);

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
     * get the representation into given stream
     */
    void representation(std::ostream &into);

    /**
     * flattens the file by deduplication
     * meaning only a single entry is considered for multiple files with same absolute path
     * returns de duplicated vector that contains entries from nested files and a entry for self at the last index
     */
    std::vector<FlatIGFile> flatten_by_dedupe();

};

/**
 * performs the same as flatten_by_dedupe on a single IGFile, however it operates on multiple files
 * it sorts them properly
 */
std::vector<FlatIGFile> flatten_by_dedupe(std::vector<IGFile>& files);

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
 * determines the import graph, returning the root ig file
 */
IGFile determine_import_graph_file(
        ImportGraphImporter* importer,
        std::vector<CSTToken*> &tokens,
        FlatIGFile &file
);

/**
 * determines the import graph
 */
IGResult determine_import_graph(
    ImportGraphImporter* importer,
    std::vector<CSTToken*> &tokens,
    FlatIGFile &file
);

/**
 * determines import graph from the given cst tokens
 */
IGResult determine_import_graph(
    const std::string &exe_path,
    std::vector<CSTToken*> &tokens,
    LocationManager& manager,
    FlatIGFile &file
);

/**
 * this determines the IG file, the root if file for the given absolute_path
 */
IGFile determine_ig_file(ImportPathHandler &path_handler, const std::string &abs_path);

/**
 * this determines the IG file, the root if file for the given absolute_path
 */
IGFile determine_ig_file(const std::string &exe_path, const std::string &abs_path);

/**
 * determines import graph, which is data structure
 */
IGResult determine_import_graph(ImportPathHandler &path_handler, const std::string &abs_path);

/**
 * determines import graph, which is data structure
 */
IGResult determine_import_graph(const std::string &exe_path, const std::string &abs_path);

/**
 * construct's a list representation from the given IGFile
 */
void representation(const IGFile &file, std::ostream &into, unsigned int level = 0);

/**
 * print the representation of the files into given stream
 */
void representation(std::ostream &into, const std::vector<IGFile>& files);

/**
 * get the representation of the files
 */
std::string representation(const std::vector<IGFile>& files);

/**
 * prints errors in the given file
 */
void print_errors(const IGFile& file);

/**
 * prints the errors in the given ig files
 */
void print_errors(const std::vector<IGFile>& files);