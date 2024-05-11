// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

class ASTNode;

/**
 * ASTLinker provides a way for the nodes to be linked
 * SemanticLinker however provides a way for the tokens to be linked
 * This doesn't link up modules like Linker does which is used for exporting executables
 */
class SymbolResolver {
public:

    /**
     * is the codegen for arch 64bit
     */
    bool is64Bit;

    /**
     * it will benchmark lexing process
     */
    bool benchmark = false;

    /**
     * constructor
     */
    SymbolResolver(const std::string& path, bool is64Bit);

    /**
     * similar to codegen it also has imported map
     * TODO delete this map, as symbol resolver needs to be merged into codegen
     * TODO only one implementation of imported map should exist anyway
     */
    std::unordered_map<std::string, bool> imported;

    /**
     * when traversing nodes, a node can declare itself on the map
     * this is vector of scopes, the last scope is current scope
     */
    std::vector<std::unordered_map<std::string, ASTNode*>> current = {{}};

    /**
     * errors occurred during linking
     */
    std::vector<std::string> errors;

    /**
     * path to the root file, were symbol resolution started
     */
    std::string path;

    /**
     * path to the current file, in which symbols are being resolved
     */
    std::string current_path;

    /**
     * when a scope beings, this should be called
     * it would put a unordered_map on current vector
     */
    void scope_start();

    /**
     * when a scope ends, this should be called
     * it would pop a scope map from the current vector
     */
    void scope_end();

    /**
     * find a symbol on current symbol map
     */
    ASTNode *find(const std::string &name);

    /**
     * declares a node with string : name
     */
    void declare(const std::string &name, ASTNode *node);

    /**
     * save an error
     */
    inline void error(const std::string &err) {
        errors.push_back(err);
    }

    /**
     * save a warning
     */
    inline void warn(const std::string &wrn) {
        errors.push_back(wrn);
    }

    /**
     * save a information
     */
    inline void info(const std::string &inf) {
        errors.push_back(inf);
    }

};