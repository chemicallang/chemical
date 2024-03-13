// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

class ASTNode;

/**
 * ASTLinker provides a way for the nodes to be linked
 * SemanticLinker however provides a way for the tokens to be linked
 */
class ASTLinker {
public:

    /**
     * nodes
     */
    std::vector<std::unique_ptr<ASTNode>> nodes;

    /**
     * constructor
     * @param nodes
     */
    ASTLinker(std::vector<std::unique_ptr<ASTNode>> nodes);

    /**
     * When traversing nodes, A node can declare something e.g a variable using variable statement
     * the identifier for the node is put on this map, so other upcoming nodes can find the node
     */
    std::unordered_map<std::string, ASTNode*> current;

};