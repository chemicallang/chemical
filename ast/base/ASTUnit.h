// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include "ast/structures/FileScope.h"

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
     * file scope contains the file scope
     */
    FileScope scope;

    /**
     * modules in which this ast unit has been declared in,
     * this is necessary because a file is supposed to be declared in a module once
     */
    std::unordered_map<LabModule*, bool> declared_in;

    /**
     * empty constructor
     */
    ASTUnit(
        unsigned int file_id,
        const chem::string_view& file_path,
        ModuleScope* modScope
    ) : scope(file_id, file_path, modScope) {

    }

    /**
     * deleted copy constructor
     */
    ASTUnit(const ASTUnit& other) = delete;

    /**
     * default move constructor
     */
    ASTUnit(ASTUnit&& other) noexcept = default;

    /**
     * move assignment operator
     */
    ASTUnit& operator =(ASTUnit&& other) noexcept = default;

    /**
     * the destructor
     */
    ~ASTUnit() = default;

};