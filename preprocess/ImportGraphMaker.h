// Copyright (c) Qinetik 2024.

#include <string>
#include <vector>
#include "common/Diagnostic.h"
#include "utils/functionalfwd.h"

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
    IGFile* parent;

    /**
     * absolute path to the file
     */
    std::string abs_path;

    /**
     * the files imported by this file
     */
    std::vector<IGFile> files;

    /**
     * get a list representation from this file
     */
    std::string representation();

    /**
     * traverse the file depth first
     */
    bool depth_first(const std::function<bool(IGFile*)>& fn);

    /**
     * traverse the file breath first
     */
    bool breath_first(const std::function<bool(IGFile*)>& fn);

};

/**
 * ImportGraph is constructed to analyse / process dependencies of a source file
 */
struct IGResult {

    /**
     * the root import graph file
     */
    IGFile root;

    /**
     * errors that occurred
     */
    std::vector<Diag> errors;

};

/**
 * determines import graph, which is data structure
 */
IGResult determine_import_graph(const std::string& exe_path, const std::string& abs_path);

/**
 * construct's a list representation from the given IGFile
 */
void representation(IGFile& file, std::string& into, unsigned int level = 0);