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
     * an anonymous module scope is used for the anonymous file
     */
    ModuleScope modScope;

    /**
     * meta data about the file
     */
    ASTFileMetaData metaData;

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
    ) : allocator(10000 /** 10 kb **/),
        metaData(fileId, &modScope, std::move(abs_path)),
        modScope("", "", nullptr), unit(metaData, &modScope)
    {

    }

    /**
     * get absolute path of the file
     */
    const std::string& getAbsPath() {
        return metaData.abs_path;
    }


};