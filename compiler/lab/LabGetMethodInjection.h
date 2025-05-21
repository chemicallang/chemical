// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

class FunctionDeclaration;
class VarInitStatement;
class ASTAllocator;
class ASTNode;

VarInitStatement* default_build_lab_build_flag(ASTAllocator& allocator, TypeBuilder& builder, ASTNode* parent);

VarInitStatement* default_build_lab_cached_ptr(ASTAllocator& allocator, TypeBuilder& builder, ASTNode* parent);

FunctionDeclaration* default_build_lab_get_method(
        ASTAllocator& allocator,
        TypeBuilder& builder,
        ASTNode* parent,
        const chem::string_view& buildFlagName,
        const chem::string_view& cachedPtrName
);