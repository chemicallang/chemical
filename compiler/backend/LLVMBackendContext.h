// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/lab/BackendContext.h"

class Codegen;

class LLVMBackendContext : public BackendContext {
public:

    Codegen* gen_ptr;

    LLVMBackendContext(Codegen* gen) : gen_ptr(gen){

    }

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
     */
    void atomic_fence(BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope, SourceLocation location) final;

    /**
     * atomic load instruction intrinsic
     */
    Value* atomic_load(Value* ptr, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final;

    /**
     * atomic store instruction intrinsic
     */
    void atomic_store(Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final;

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

};