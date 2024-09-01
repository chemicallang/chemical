// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include "ast/structures/Scope.h"

class ASTNode;
class BaseType;
class Value;

/**
 * an ASTUnit is the complete AST of a single file (mostly)
 * It represents everything allocated after parsing the tokens
 */
class ASTUnit {
public:

    /**
     * The top level scope, This contains nodes that have raw pointers
     * to values and types allocated in this ASTUnit
     */
    Scope scope;

    /**
     * types allocated during parsing, they don't have any organization to them
     */
    std::vector<std::unique_ptr<BaseType>> types;

    /**
     * values allocated during parsing, they don't have any organization to them
     */
    std::vector<std::unique_ptr<Value>> values;

    /**
     * nested nodes allocated during parsing, they don't have any organization to them
     */
    std::vector<std::unique_ptr<ASTNode>> nested_nodes;

    /**
     * empty constructor
     */
    ASTUnit();

    /**
     * deleted copy constructor
     */
    ASTUnit(const ASTUnit& other) = delete;

    /**
     * default move constructor
     */
    ASTUnit(ASTUnit&& other) = default;

    /**
     * move assignment operator
     */
    ASTUnit& operator =(ASTUnit&& other) = default;

    /**
     * the destructor
     */
    ~ASTUnit();

};