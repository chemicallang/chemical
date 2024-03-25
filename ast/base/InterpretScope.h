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

using node_map = std::unordered_map<std::string, ASTNode*>;
using node_iterator = node_map::iterator;
using value_map = std::unordered_map<std::string, Value*>;
using value_iterator = value_map::iterator;

class InterpretScope {
public:

    /**
     * explicit constructor
     */
    explicit InterpretScope(InterpretScope* parent, GlobalInterpretScope* global, Scope* scope, ASTNode* node);

    /**
     * use default move constructor
     */
    InterpretScope(InterpretScope&& global) = default;

    /**
     * deleted copy constructor
     * @param copy
     */
    InterpretScope(const InterpretScope& copy) = delete;

    /**
     * declares a value with this name in current scope
     */
    void declare(const std::string& name, Value* value);

    /**
     * declares a node with this name in current scope
     */
    void declare(const std::string& name, ASTNode* node);

    /**
     * erases a value by the key name from the value map safely
     */
    void erase_value(const std::string &name);

    /**
     * erases a name by the key name from the node map safely
     */
    void erase_node(const std::string &name);

    /**
     * @return return node with name, or nullptr
     */
    ASTNode* find_node(const std::string& name);

    /**
     * @return return value with name, or nullptr
     */
    Value* find_value(const std::string& name);

    /**
     * @return iterator for found node, the map that it was found in
     */
    std::pair<node_iterator, node_map&> find_node_iterator(const std::string& name);

    /**
     * @return iterator for found value, the map that it was found in
     */
    std::pair<value_iterator, value_map&> find_value_iterator(const std::string& name);

    /**
     * print all values
     */
    void print_values();

    /**
     * print all nodes
     */
    void print_nodes();

    /**
     * The errors are stored in global scope only
     * @param err
     */
    void error(const std::string& err);

    /**
     * this can be called to "clean" everything in this scope
     * to make it reusable
     */
    virtual void clean();

    /**
     * Values that want to be deleted when the scope ends
     * must be deleted in the destructor
     */
    virtual ~InterpretScope();

    /**
      * This contains a map between identifiers and its values, of the current scope
      */
    std::unordered_map<std::string, Value *> values;

    /**
     * When a ASTNode declares itself, for example a struct, interface / implementation
     * it declares itself on this unordered map, when the scope ends, it erases itself from this map
     */
    std::unordered_map<std::string, ASTNode *> nodes;

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