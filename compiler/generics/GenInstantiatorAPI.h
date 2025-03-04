// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ast_fwd.h"

FunctionDeclaration* Instantiate(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, GenericFuncDecl* decl, size_t itr);

StructDefinition* Instantiate(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, GenericStructDecl* decl, size_t itr);