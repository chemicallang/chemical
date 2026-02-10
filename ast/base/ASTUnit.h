// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/structures/FileScope.h"

class BaseType;
class Value;
struct LabModule;

/**
 * an ASTUnit is the complete AST of a single file
 * It represents everything allocated after parsing the tokens
 */
class ASTUnit {
public:

    /**
     * file scope contains the file scope
     */
    FileScope scope;

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
     * update module scope this unit belongs to
     */
    void set_parent(ModuleScope* modScope) {
        scope.set_parent(modScope);
    }

    /**
     * the destructor
     */
    ~ASTUnit() = default;

};