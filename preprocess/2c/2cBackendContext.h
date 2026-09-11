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

    bool supports(CompilerFeatureKind kind) final;


    void destruct_call_site(SourceLocation location) final {
        visitor->destruct_scopes_above(nullptr);
    }

    /**
     * atomic fence
     * @return a value the caller should render in place (an empty RawLiteral
     * when the fence was emitted eagerly; never nullptr for the C backend)
     */
    Value* atomic_fence(BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope, SourceLocation location) final;

    /**
     * atomic load instruction intrinsic
     */
    Value* atomic_load(Value* ptr, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final;

    /**
     * atomic store instruction intrinsic
     * @return a value the caller should render in place (an empty RawLiteral
     * when the store was emitted eagerly; never nullptr for the C backend)
     */
    Value* atomic_store(Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final;

    /**
     * atomic compare exchange weak — weak=1 (may spuriously fail)
     */
    Value* atomic_cmp_exch_weak(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) final;

    /**
     * atomic compare exchange strong — weak=0 (never spuriously fails)
     */
    Value* atomic_cmp_exch_strong(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) final;

    /**
     * atomic operation, supports add, sub, and, or, xor
     */
    Value* atomic_op(BackendAtomicOp op, Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) final;

    /**
     * compiler-only fence (empty-asm statement)
     * @return a value the caller should render in place (an empty RawLiteral
     * when the asm was emitted eagerly; never nullptr for the C backend)
     */
    Value* signal_fence(BackendAtomicMemoryOrder order) final;

};