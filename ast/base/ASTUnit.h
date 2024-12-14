// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include "ast/structures/Scope.h"

class ASTNode;
class BaseType;
class Value;
class LabModule;

/**
 * an ASTUnit is the complete AST of a single file (mostly)
 * It represents everything allocated after parsing the tokens
 */
class ASTUnit {
public:

    /**
     * this allocator was used to serially allocate strings in the lexer or parser
     */
    BatchAllocator serial_str_alloc;

    /**
     * The top level scope, This contains nodes that have raw pointers
     * to values and types allocated in this ASTUnit
     */
    Scope scope;

    /**
     * modules in which this ast unit has been declared in,
     * this is necessary because a file is supposed to be declared in a module once
     */
    std::unordered_map<LabModule*, bool> declared_in;

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
    ASTUnit(ASTUnit&& other) noexcept ;

    /**
     * move assignment operator
     */
    ASTUnit& operator =(ASTUnit&& other) noexcept ;

    /**
     * the destructor
     */
    ~ASTUnit();

};