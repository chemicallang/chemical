// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class FunctionDeclaration;
class ASTAllocator;
class ASTDiagnoser;
class GenericFuncDecl;

FunctionDeclaration* Instantiate(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, GenericFuncDecl* decl, size_t itr);