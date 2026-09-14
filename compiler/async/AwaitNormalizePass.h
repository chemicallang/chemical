// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "compiler/async/AsyncLoweringPlan.h"

class ASTAllocator;
class FunctionDeclaration;
class LambdaFunction;
class Scope;

/**
 * Rewrites an async function/closure body so that every `await` becomes the
 * initializer of a `VarInitStatement`, hoisting nested awaits in front of the
 * enclosing statement while preserving left-to-right evaluation order
 * (design Sections 7, 7.6).
 *
 * The rewrite is idempotent and only runs inside async bodies. It is a
 * behavior-preserving transformation for the current eager bootstrap; the real
 * coroutine consumers (Phases 3/4) rely on the resulting invariant:
 *
 *   every AwaitExpression is the initializer of a VarInitStatement.
 *
 * Returns true if the body was modified.
 */
bool normalize_async_body(ASTAllocator& allocator, FunctionDeclaration* decl);

/**
 * Same normalization for an async closure body.
 */
bool normalize_async_lambda(ASTAllocator& allocator, LambdaFunction* lambda);

/**
 * Builds the shared lowering plan for an async function body. Must be called
 * after normalization. Safe to call for non-async functions (yields an empty
 * plan).
 */
AsyncLoweringPlan build_async_plan(FunctionDeclaration* decl);
