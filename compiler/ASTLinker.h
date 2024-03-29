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
class ASTLinker {
public:

    /**
     * constructor
     */
    ASTLinker();

    /**
     * When traversing nodes, A node can declare something e.g a variable using variable statement
     * the identifier for the node is put on this map, so other upcoming nodes can find the node
     */
    std::unordered_map<std::string, ASTNode*> current;

    /**
     * errors occurred during linking
     */
    std::vector<std::string> errors;

    /**
     * save an error
     */
    inline void error(const std::string& err) {
        errors.push_back(err);
    }

};