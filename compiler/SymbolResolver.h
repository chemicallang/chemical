// Copyright (c) Qinetik 2024.

#pragma once

#include "preprocess/BaseSymbolResolver.h"
#include "ASTDiagnoser.h"

class ASTNode;

/**
 * ASTLinker provides a way for the nodes to be linked
 * SemanticLinker however provides a way for the tokens to be linked
 * This doesn't link up modules like Linker does which is used for exporting executables
 */
class SymbolResolver : public BaseSymbolResolver<ASTNode>, public ASTDiagnoser {
public:

    /**
     * is the codegen for arch 64bit
     */
    bool is64Bit;

    /**
     * when true, re-declaring same symbol will override it
     */
    bool override_symbols = false;

    /**
     * constructor
     */
    SymbolResolver(std::string curr_exe_path, const std::string& path, bool is64Bit);

    /**
     * similar to codegen it also has imported map
     * TODO delete this map, as symbol resolver needs to be merged into codegen
     * TODO only one implementation of imported map should exist anyway
     */
    std::unordered_map<std::string, bool> imported;

    /**
     * declares a node with string : name
     */
    void declare(const std::string &name, ASTNode *node);

    /**
     * tag
     */
    std::string TAG() override {
        return "SymRes";
    }

};