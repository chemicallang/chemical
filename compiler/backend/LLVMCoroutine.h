// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#ifdef COMPILER_BUILD

#include <cstdint>

namespace llvm {
    class Value;
    class Function;
    class StructType;
    class BasicBlock;
    class Type;
}

class Codegen;
class FunctionDeclaration;
class BaseType;
class AwaitExpression;

/**
 * Per-async-function state for the LLVM coroutine lowering (design Section 9).
 * Installed on `Codegen::current_coro` while the body of a lowered async
 * function is generated, so `AwaitExpression` and `return` can emit the coroutine
 * suspend/completion sequence instead of the normal code.
 */
struct LLVMCoroContext {

    FunctionDeclaration* decl = nullptr;

    // the ramp function (the user-visible symbol); returns `FutureHandle<T>` via sret
    llvm::Function* ramp = nullptr;

    // the handle sret pointer (first argument of the ramp)
    llvm::Value* handle_sret = nullptr;

    // the coroutine frame base and promise pointer
    llvm::Value* frame = nullptr;     // @llvm.coro.begin result
    llvm::Value* promise = nullptr;   // @llvm.coro.promise result
    llvm::Value* id = nullptr;        // @llvm.coro.id token

    llvm::StructType* promise_ty = nullptr;
    unsigned promise_align = 8;
    unsigned promise_offset = 0;   // byte offset of the promise from the frame base

    // the frame size/alignment as i64 values, used to free the frame
    llvm::Value* frame_size = nullptr;
    llvm::Value* frame_align = nullptr;

    BaseType* inner = nullptr;        // T
    llvm::Type* inner_ty = nullptr;   // LLVM type of T
    llvm::Type* poll_ty = nullptr;    // Poll<T>
    llvm::Type* handle_ty = nullptr;  // FutureHandle<T>

    // 0 = running, 1 = suspended, 2 = completed
    llvm::Value* state_ptr = nullptr;

    llvm::BasicBlock* final_suspend_bb = nullptr;
    llvm::BasicBlock* cleanup_bb = nullptr;

    // shared return-to-caller path (`coro.end(i1 false); ret`); the default arm
    // of every suspend routes here so a real suspension returns to the caller
    llvm::BasicBlock* ramp_ret_bb = nullptr;

    unsigned result_field = 2;
    unsigned cx_field = 1;
    unsigned state_field = 1;

    /**
     * Opaque pointer to the `AsyncFrameLayout` computed by `gen_llvm_async_fn`
     * (valid for the duration of the body generation). Used by `gen_llvm_await`
     * to store the child future and set the suspension state.
     */
    const void* frame_layout = nullptr;
};

/**
 * Emits the LLVM lowering of an async function: the coroutine ramp plus its
 * `poll`/`drop` functions and the `FutureTable<T>` vtable. Returns false when the
 * function is not a lowered async function (so the caller falls back to the
 * normal body emission).
 */
bool gen_llvm_async_fn(Codegen& gen, FunctionDeclaration* decl);

/**
 * Lowers `await e` inside a lowered async function (design Section 9.6). Drives
 * the child future's `poll` through its vtable and, when pending, suspends the
 * coroutine. Returns the awaited value.
 */
llvm::Value* gen_llvm_await(Codegen& gen, AwaitExpression* await);

#endif // COMPILER_BUILD
