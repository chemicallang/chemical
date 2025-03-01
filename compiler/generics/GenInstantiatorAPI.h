// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class FunctionDeclaration;
class ASTAllocator;
class GenericFuncDecl;

FunctionDeclaration* Instantiate(ASTAllocator& astAllocator, GenericFuncDecl* decl, size_t itr);