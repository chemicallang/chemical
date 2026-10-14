// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/lab/BackendContext.h"

/**
 * The LLVM target's `BackendContext`.
 *
 * The `BackendContext` hooks are the interface the comptime interpreter uses when
 * a comptime function calls an intrinsic with a per-backend implementation
 * (`intrinsics::forget`, `intrinsics::memcpy`, the atomic builtins). MIR owns all
 * of those behaviours now — the MIR → LLVM emitter lowers moves/copies, drop flags
 * and atomics as first-class MIR ops — so the hooks are only reached for a
 * *compile-time* evaluation and are inert, exactly like the C backend's.
 *
 * `name` and `supports` are real answers, not stubs: they drive
 * `intrinsics::get_backend_name` and `intrinsics::supports`.
 */
class LLVMBackendContext : public BackendContext {
public:

    chem::string_view name() final {
        return "LLVM";
    }

    void emit(const chem::string_view& value) {
        // no support for emitting the string
        // we could support some small codes here
    }

    bool forget(ASTNode* node) final;

    void mem_copy(Value* lhs, Value* rhs) final;

    bool supports(CompilerFeatureKind kind) final {
        return true;
    }

    void destruct_call_site(SourceLocation location) final;

    /**
     * atomic fence
     * @return a value the caller should render in place (nullptr — MIR lowers
     * atomics as first-class ops, so nothing is rendered here)
     */
    Value* atomic_fence(BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope, SourceLocation location) final;

    /**
     * atomic load instruction intrinsic
     */
    Value* atomic_load(Value* ptr, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final;

    /**
     * atomic store instruction intrinsic
     * @return a value the caller should render in place (nullptr)
     */
    Value* atomic_store(Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final;

    /**
     * atomic compare exchange weak
     */
    Value* atomic_cmp_exch_weak(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) final;

    /**
     * atomic compare exchange strong
     */
    Value* atomic_cmp_exch_strong(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) final;

    /**
     * atomic operation, supports add, sub, and, or, xor
     */
    Value* atomic_op(BackendAtomicOp op, Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final;

    /**
     * compiler-only fence
     * @return a value the caller should render in place (nullptr)
     */
    Value* signal_fence(BackendAtomicMemoryOrder order) final;

};
