// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTAllocator.h"
#include "ast/base/ASTUnit.h"
#include "ast/structures/ModuleScope.h"

/**
 * anonymous file is a file that doesn't belong to a module, is opened
 * in ide and is processed by lsp
 */
class AnonymousFileData {
public:

    /**
     * the allocator is used to allocate the file
     */
    ASTAllocator allocator;

    /**
     * the path to file
     */
    std::string abs_path;

    /**
     * an anonymous module scope is used for the anonymous file
     */
    ModuleScope modScope;

    /**
     * the unit that is allocated
     */
    ASTUnit unit;

    /**
     * constructor
     */
    AnonymousFileData(
            unsigned int fileId,
            std::string abs_path
    ) : allocator(10000 /** 10 kb **/), abs_path(std::move(abs_path)),
        modScope("", "", nullptr), unit(fileId, chem::string_view(this->abs_path), &modScope)
    {

    }


};