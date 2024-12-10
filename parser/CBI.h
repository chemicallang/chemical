// Copyright (c) Qinetik 2024.

#pragma once

class Value;

class Parser;

class ASTAllocator;

/**
 * function is used to parse #sizeof values
 */
Value* parseSizeOfValue(Parser* parser, ASTAllocator* allocator);

/**
 * function is used to parse #eval values
 */
Value* parseEvalValue(Parser* parser, ASTAllocator* allocator);