// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <span>

class ASTDiagnoser;

class ASTAllocator;

class Value;

class BaseType;

class ImplementationsIndex;

void unsatisfied_type_err(ASTDiagnoser& diagnoser, ASTAllocator& allocator, Value* value, BaseType* type);

void type_verify(
    ImplementationsIndex& index,
    ASTDiagnoser& diagnoser,
    ASTAllocator& allocator,
    std::span<ASTNode*> nodes
);