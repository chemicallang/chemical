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
    constexpr ASTUnit(
        ASTFileMetaData& meta,
        ModuleScope* modScope
    ) : scope(meta, modScope) {

    }

    /**
     * deleted copy constructor
     */
    ASTUnit(const ASTUnit& other) = delete;

    /**
     * update module scope this unit belongs to
     */
    void set_parent(ModuleScope* modScope) {
        scope.set_parent(modScope);
    }

};