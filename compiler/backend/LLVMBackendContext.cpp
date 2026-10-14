// Copyright (c) Chemical Language Foundation 2025.
//
// These hooks used to emit AST-lowered IR from `backend/LLVM.cpp`, which has been
// deleted. MIR lowers moves/copies, destructor call sites and atomics as
// first-class ops (see `MIRLowerer::lower_atomic_intrinsic` and the
// `Drop`/`Destroy`/`MemCpy` opcodes), so this class holds no state and the hooks
// below are inert; `forget` mirrors the C backend (which is also a no-op because
// MIR drop flags decide destruction).

#include "LLVMBackendContext.h"

bool LLVMBackendContext::forget(ASTNode*) {
    // MIR owns destruction via drop flags; nothing to forget here.
    return false;
}

void LLVMBackendContext::mem_copy(Value*, Value*) {
    // MIR emits struct copies directly; the legacy comptime hook is unused.
}

void LLVMBackendContext::destruct_call_site(SourceLocation) {}

Value* LLVMBackendContext::atomic_fence(BackendAtomicMemoryOrder, BackendAtomicSyncScope,
                                        SourceLocation) {
    return nullptr;
}

Value* LLVMBackendContext::atomic_load(Value* ptr, BackendAtomicMemoryOrder,
                                       BackendAtomicSyncScope) {
    return ptr;
}

Value* LLVMBackendContext::atomic_store(Value*, Value*, BackendAtomicMemoryOrder,
                                        BackendAtomicSyncScope) {
    return nullptr;
}

Value* LLVMBackendContext::atomic_cmp_exch_weak(Value* ptr, Value*, Value*,
                                                BackendAtomicMemoryOrder,
                                                BackendAtomicMemoryOrder,
                                                BackendAtomicSyncScope) {
    return ptr;
}

Value* LLVMBackendContext::atomic_cmp_exch_strong(Value* ptr, Value* expected, Value* value,
                                                  BackendAtomicMemoryOrder success_order,
                                                  BackendAtomicMemoryOrder failure_order,
                                                  BackendAtomicSyncScope scope) {
    return atomic_cmp_exch_weak(ptr, expected, value, success_order, failure_order, scope);
}

Value* LLVMBackendContext::atomic_op(BackendAtomicOp, Value* ptr, Value*, BackendAtomicMemoryOrder,
                                     BackendAtomicSyncScope) {
    return ptr;
}

Value* LLVMBackendContext::signal_fence(BackendAtomicMemoryOrder) {
    return nullptr;
}
