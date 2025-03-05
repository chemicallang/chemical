// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ast_fwd.h"

/**
 * instantiate a function declaration
 */
FunctionDeclaration* Instantiate(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, GenericFuncDecl* decl, size_t itr);

/**
 * instantiate a struct declaration
 */
StructDefinition* Instantiate(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, GenericStructDecl* decl, size_t itr);