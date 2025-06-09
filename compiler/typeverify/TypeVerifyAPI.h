// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <span>

class ASTDiagnoser;

class ASTAllocator;

class Value;

class BaseType;

void unsatisfied_type_err(ASTDiagnoser& diagnoser, ASTAllocator& allocator, Value* value, BaseType* type);

void type_verify(ASTDiagnoser& diagnoser, ASTAllocator& allocator, std::span<ASTNode*> nodes);