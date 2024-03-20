// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <string>
#include <unordered_map>

class Value;

class GlobalInterpretScope;

class Scope;

class ASTNode;

class InterpretScope {
public:

    explicit InterpretScope(InterpretScope* parent, GlobalInterpretScope* global, Scope* scope, ASTNode* node);

    // delete copy constructor
    InterpretScope(InterpretScope&& copy) = delete;

    /**
     * The errors are stored in global scope only
     * @param err
     */
    void error(const std::string& err);

    /**
     * Values that want to be deleted when the scope ends
     * must be deleted in the destructor
     */
    ~InterpretScope();

    /**
     * a pointer to the parent scope, If this is a global scope, it will be a nullptr
     */
    InterpretScope* parent;

    /**
     * a reference to the current code scope
     */
     Scope* codeScope;

     /**
      * a reference to the holder ast node, Its nullptr in a global scope
      */
     ASTNode* node;

    /**
     * a pointer to global scope, If this is a global scope, it will be a pointer to itself
     */
    GlobalInterpretScope* global;

    /**
     * this just holds the counter of AST nodes interpreted in the current scope
     * why is this useful -> each node can create values on the stack, we must remove these values when scope ends
     * if we stop in the middle and don't interpret rest of the nodes (loop break, function return), we can't ask
     * all the nodes to cleanup, we only cleanup the nodes till we stopped interpreting
     * if a single node is interpreted, its incremented to 0
     * when cleaning up, we loop until its bigger than 0
     */
    int nodes_interpreted = -1;

};