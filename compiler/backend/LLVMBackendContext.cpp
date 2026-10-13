// Copyright (c) Chemical Language Foundation 2025.
//
// `LLVMBackendContext` is the LLVM target's `BackendContext` (it answers
// `intrinsics::supports`, `intrinsics::backend_name`, and the comptime
// `intrinsics::forget` hook). Its former out-of-line definitions lived in the
// AST-based `backend/LLVM.cpp`, which has been deleted.
//
// The hooks that emitted AST-lowered IR (`mem_copy`, `destruct_call_site`, and
// the `atomic_*` intrinsics) are unreachable now: the MIR→LLVM path lowers
// atomics, moves/copies and destructor call sites as first-class MIR ops (see
// `MIRLowerer::lower_atomic_intrinsic` and the `Drop`/`Destroy`/`MemCpy` ops),
// so they are provided here only to complete the vtable. `forget` is kept
// faithful — it is reached at comptime by `intrinsics::forget` (used by
// `std::vector`, `std::deque`, `json`, …) and only touches `Codegen`'s
// destructible list.

#include "LLVMBackendContext.h"

#include "compiler/Codegen.h"
#include "ast/base/ASTNode.h"

#include <algorithm>

bool LLVMBackendContext::forget(ASTNode* targetNode) {
    auto& gen = *gen_ptr;
    auto it = std::find_if(
            gen.destruct_nodes.begin(),
            gen.destruct_nodes.end(),
            [targetNode](auto& p) {
                return p.getInitializer() == targetNode;
            });
    if (it == gen.destruct_nodes.end()) return false;
    gen.destruct_nodes.erase(it);
    return true;
}

// The remaining hooks served the deleted AST lowering. The MIR path handles
// their effects itself, so they are unreachable; they exist only for the vtable.
void LLVMBackendContext::mem_copy(Value*, Value*) {}

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
