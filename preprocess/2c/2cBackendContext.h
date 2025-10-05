// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/lab/BackendContext.h"

class ToCAstVisitor;

class ToCBackendContext : public BackendContext {
public:

    ToCAstVisitor* visitor;

    ToCBackendContext(ToCAstVisitor* visitor) : visitor(visitor) {

    }

    chem::string_view name() final {
        return "C";
    }

    void emit(const chem::string_view &value) override {
        // direct write to output
        visitor->write(value);
    }

    bool forget(ASTNode* node) final;

    void mem_copy(Value *lhs, Value *rhs) final;

    bool supports(CompilerFeatureKind kind) final {
        switch(kind) {
            case CompilerFeatureKind::Float128:
                return false;
            default:
                return true;
        }
    }

    void destruct_call_site(SourceLocation location) final {
        visitor->destruct_current_scope(nullptr);
    }

    /**
     * atomic fence
     */
    void atomic_fence(BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope, SourceLocation location) final {
        // not supported
    }

    /**
     * atomic load instruction intrinsic
     */
    Value* atomic_load(Value* ptr, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final {
        // not supported
        return ptr;
    }

    /**
     * atomic store instruction intrinsic
     */
    void atomic_store(Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final {
        // not supported
    }

    /**
     * atomic compare exchange weak
     */
    Value* atomic_cmp_exch_weak(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) final {
        // not supported
        return value;
    }

    /**
     * atomic compare exchange strong
     */
    Value* atomic_cmp_exch_strong(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) final {
        // not supported
        return value;
    }

    /**
     * atomic operation, supports add, sub, and, or, xor
     */
    Value* atomic_op(BackendAtomicOp op, Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final {
        // not supported
        return value;
    }


};