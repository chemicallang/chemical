// Copyright (c) Chemical Language Foundation 2026.

#ifdef COMPILER_BUILD

#include "LLVMCoroutine.h"

#include "compiler/Codegen.h"
#include "compiler/async/AwaitNormalizePass.h"
#include "compiler/async/AsyncLoweringPlan.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/FunctionType.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/ArrayType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/structures/Scope.h"
#include "ast/statements/VarInit.h"
#include "ast/values/AwaitExpression.h"
#include "ast/base/Value.h"

#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <vector>

using namespace llvm;

/**
 * Layout of the compiler-owned coroutine frame produced by `gen_llvm_async_fn`:
 * the pinned storage field and drop-flag field index of each frame slot, and the
 * child-future field of each await site. Consumed by `gen_llvm_await` (to store
 * the child and set the state) and by `emit_drop_fn` (to run the cancellation
 * cleanup — the LLVM analogue of the 2c backend's `__drop` state switch).
 */
struct AsyncFrameLayout {
    const AsyncLoweringPlan* plan = nullptr;
    std::vector<unsigned> slot_field;                 // slot id -> wrapper field index (0 = not pinned)
    std::vector<unsigned> flag_field;                 // slot id -> drop flag field index (0 = none)
    std::vector<unsigned> child_field;                // site   -> child handle field index (0 = none)
    std::vector<llvm::Type*> child_handle_ty;         // site   -> FutureHandle<T> llvm type
};

/**
 * Emits the destructor of a frame-resident value of `type` at `ptr` (element-wise
 * for arrays of destructor-bearing elements). Used by the cancellation cleanup.
 */
static void emit_destroy_type(Codegen& gen, BaseType* type, llvm::Value* ptr, SourceLocation loc) {
    if(type == nullptr || ptr == nullptr) {
        return;
    }
    auto* canonical = type->canonical();
    if(canonical->kind() == BaseTypeKind::Array) {
        auto* arr = canonical->as_array_type_unsafe();
        if(!arr->has_array_size() || arr->elem_type == nullptr) {
            return;
        }
        auto* elem = arr->elem_type->canonical();
        if(elem->get_destructor() == nullptr) {
            return;
        }
        Destructible d{};
        d.kind = DestructibleKind::Array;
        d.initializer = nullptr;
        d.dropFlag = nullptr;
        d.pointer = ptr;
        d.array.arrSize = arr->get_array_size();
        d.array.elem_type = elem;
        gen.destruct(d, loc);
        return;
    }
    auto* destr = type->get_destructor();
    if(destr == nullptr) {
        return;
    }
    gen.builder->CreateCall(destr->llvm_func(gen), {ptr});
}

static llvm::Value* gep_idx(IRBuilder<>& b, llvm::Type* ty, llvm::Value* ptr, std::initializer_list<unsigned> idxs) {
    std::vector<llvm::Value*> list;
    list.reserve(idxs.size());
    for(unsigned i : idxs) {
        list.push_back(b.getInt32(i));
    }
    return b.CreateGEP(ty, ptr, list);
}

/**
 * True when the function's return type is the compiler-generated
 * `FutureHandle<T>` (i.e. symres wrapped it for a lowered async function).
 */
static bool is_future_handle_type(BaseType* rt, BaseType*& inner) {
    if(rt == nullptr || rt->kind() != BaseTypeKind::Generic) {
        return false;
    }
    auto gen = rt->as_generic_type_unsafe();
    if(gen->types.empty() || gen->referenced == nullptr || gen->referenced->linked == nullptr) {
        return false;
    }
    const auto linked = gen->referenced->linked;
    if(linked->kind() != ASTNodeKind::StructDecl) {
        return false;
    }
    if(linked->as_struct_def_unsafe()->name_view() != chem::string_view("FutureHandle")) {
        return false;
    }
    inner = const_cast<BaseType*>(gen->types[0].getType());
    return inner != nullptr;
}

/**
 * Resolves the `FutureTable<T>` BaseType from a `FutureHandle<T>` type by
 * walking the handle's `vtbl` field. Returns nullptr when the shape cannot be
 * resolved.
 */
static BaseType* resolve_future_table_from_handle(BaseType* rt) {
    if(rt == nullptr || rt->get_direct_linked_container() == nullptr) {
        return nullptr;
    }
    auto* fh = rt->get_direct_linked_container();
    for(const auto member : fh->variables()) {
        if(member->name != chem::string_view("vtbl")) {
            continue;
        }
        auto* vt = const_cast<BaseType*>(member->as_struct_member_unsafe()->type.getType());
        if(vt->kind() == BaseTypeKind::Pointer) {
            return const_cast<BaseType*>(vt->as_pointer_type_unsafe()->type);
        }
        if(vt->kind() == BaseTypeKind::Reference) {
            return const_cast<BaseType*>(vt->as_reference_type_unsafe()->type);
        }
    }
    return nullptr;
}

/**
 * Resolves the concrete `Poll<T>` LLVM type from a `FutureHandle<T>` type by
 * walking the handle's vtbl into its `poll` method's return type. The child
 * future's `Poll<T>` may differ from the enclosing async function's, so await
 * lowering must use this rather than the enclosing coroutine's `poll_ty`.
 */
static llvm::Type* resolve_poll_type_from_handle(Codegen& gen, BaseType* rt) {
    BaseType* table_bt = resolve_future_table_from_handle(rt);
    if(table_bt == nullptr || table_bt->get_direct_linked_container() == nullptr) {
        return nullptr;
    }
    auto* table = table_bt->get_direct_linked_container();
    for(const auto member : table->variables()) {
        if(member->name != chem::string_view("poll")) {
            continue;
        }
        auto* ft = const_cast<BaseType*>(member->as_struct_member_unsafe()->type.getType());
        if(ft->kind() == BaseTypeKind::Function) {
            auto* poll_bt = const_cast<BaseType*>(ft->as_function_type_unsafe()->returnType.getType());
            if(poll_bt != nullptr) {
                return poll_bt->llvm_type(gen);
            }
        }
    }
    return nullptr;
}

static llvm::Function* coro_intrinsic(Codegen& gen, Intrinsic::ID id, ArrayRef<Type*> types = {}) {
    return Intrinsic::getOrInsertDeclaration(gen.module.get(), id, types);
}

/**
 * Stores a `Poll<T>` value into `dest`.
 * Layout is `{ i32 tag, { union of member structs } }`; `Ready` is member 0 with
 * its value at the start of the payload union, `Pending` is tag 1.
 */
static void emit_poll(Codegen& gen, llvm::Value* dest, llvm::Type* poll_ty, bool ready, llvm::Value* value) {
    auto& builder = *gen.builder;
    auto* tmp = builder.CreateAlloca(poll_ty);
    auto* tag_ptr = gep_idx(builder, poll_ty, tmp, {0, 0});
    builder.CreateStore(builder.getInt32(ready ? 0 : 1), tag_ptr);
    if(ready && value != nullptr) {
        auto* payload = gep_idx(builder, poll_ty, tmp, {0, 1});
        builder.CreateStore(value, payload);
    }
    const auto size = gen.module->getDataLayout().getTypeAllocSize(poll_ty);
    builder.CreateMemCpy(dest, MaybeAlign(), tmp, MaybeAlign(), size);
}

/**
 * Emits `void @foo_poll(Poll<T>* sret, void* frame, Context* cx)`.
 */
static llvm::Function* emit_poll_fn(
        Codegen& gen,
        LLVMCoroContext& coro,
        llvm::Function* resume_fn,
        llvm::Function* done_fn
) {
    auto& ctx = *gen.ctx;
    auto& builder = *gen.builder;
    auto* ptr_ty = builder.getPtrTy();
    auto* i32 = builder.getInt32Ty();

    const auto name = coro.ramp->getName().str() + "__poll";
    auto* fn_ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty, ptr_ty, ptr_ty}, false);
    auto* fn = Function::Create(fn_ty, GlobalValue::InternalLinkage, name, gen.module.get());
    auto* sret = fn->getArg(0);
    auto* frame = fn->getArg(1);   // our wrapper
    auto* cx = fn->getArg(2);

    auto* entry = BasicBlock::Create(ctx, "entry", fn);
    gen.SetInsertPoint(entry);

    // wrapper->coro is the LLVM coroutine frame
    auto* coro_frame = builder.CreateLoad(ptr_ty, gep_idx(builder, coro.promise_ty, frame, {0, 0}));
    builder.CreateStore(cx, gep_idx(builder, coro.promise_ty, frame, {0, (unsigned) coro.cx_field}));
    builder.CreateCall(resume_fn, {coro_frame});
    auto* done = builder.CreateCall(done_fn, {coro_frame});
    auto* ready_bb = BasicBlock::Create(ctx, "ready", fn);
    auto* pending_bb = BasicBlock::Create(ctx, "pending", fn);
    builder.CreateCondBr(done, ready_bb, pending_bb);

    gen.SetInsertPoint(pending_bb);
    emit_poll(gen, sret, coro.poll_ty, false, nullptr);
    builder.CreateRetVoid();

    gen.SetInsertPoint(ready_bb);
    llvm::Value* result = nullptr;
    if(!coro.inner_ty->isVoidTy()) {
        result = builder.CreateLoad(coro.inner_ty, gep_idx(builder, coro.promise_ty, frame, {0, (unsigned) coro.result_field}));
    }
    emit_poll(gen, sret, coro.poll_ty, true, result);
    builder.CreateRetVoid();
    return fn;
}

/**
 * Emits `void @foo_drop(void* frame)`.
 *
 * This is the cancellation path. It runs the destructors of the locals live at
 * the suspension site (`AsyncFrameLayout::plan->sites[i].live_drops`), guarded
 * by their frame drop flags, cancels the site's child future, and only then
 * destroys the coroutine. Running the cleanup here (a plain C-ABI function we
 * fully control) rather than in the coroutine's `i8 1` branch avoids the
 * `CoroSplit`-mangled destroy path referencing a dead local's aliased slot.
 */
static llvm::Function* emit_drop_fn(
        Codegen& gen,
        LLVMCoroContext& coro,
        const AsyncFrameLayout& layout,
        llvm::Function* destroy_fn,
        llvm::Function* frame_free_fn
) {
    auto& ctx = *gen.ctx;
    auto& builder = *gen.builder;
    auto* ptr_ty = builder.getPtrTy();
    auto* i32 = builder.getInt32Ty();
    auto* i1 = builder.getInt1Ty();
    auto* i64 = builder.getInt64Ty();
    const auto name = coro.ramp->getName().str() + "__drop";
    auto* ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty}, false);
    auto* fn = Function::Create(ty, GlobalValue::InternalLinkage, name, gen.module.get());
    auto* wrapper = fn->getArg(0);

    const auto prev_func = gen.current_function;
    gen.current_function = fn;
    auto* entry = BasicBlock::Create(ctx, "entry", fn);
    gen.SetInsertPoint(entry);

    const auto wrapper_field = [&](unsigned index) -> llvm::Value* {
        return gep_idx(builder, coro.promise_ty, wrapper, {0, index});
    };

    auto* done_bb = BasicBlock::Create(ctx, "drop.done", fn);
    // A *completed* coroutine (state 0xFFFFFFFF) has no live locals and does not
    // need its coroutine frame destroyed — running `coro.destroy` on it can
    // re-enter the body when the frame's stored resume index was not advanced
    // (B22). Free its frames directly instead.
    auto* free_bb = BasicBlock::Create(ctx, "drop.free", fn);
    auto* state = builder.CreateLoad(i32, wrapper_field((unsigned) coro.state_field));
    auto* sw = builder.CreateSwitch(state, done_bb, (layout.plan != nullptr ? layout.plan->sites.size() : 0) + 1);
    sw->addCase(ConstantInt::get(i32, 0xFFFFFFFFu), free_bb);
    if(layout.plan != nullptr) {
        for(unsigned site = 0; site < layout.plan->sites.size(); site++) {
            auto* site_bb = BasicBlock::Create(ctx, "drop.site", fn);
            sw->addCase(ConstantInt::get(i32, site + 1), site_bb);
            gen.SetInsertPoint(site_bb);
            // destroy the destructible locals live at this site (reverse order)
            const auto& live = layout.plan->sites[site].live_drops;
            for(auto it = live.rbegin(); it != live.rend(); ++it) {
                const unsigned id = *it;
                if(id >= layout.slot_field.size() || layout.slot_field[id] == 0) {
                    continue;
                }
                auto* ptr = wrapper_field(layout.slot_field[id]);
                auto* type = layout.plan->slots[id].type;
                if(layout.flag_field[id] != 0) {
                    auto* flag = builder.CreateLoad(i1, wrapper_field(layout.flag_field[id]));
                    auto* yes = BasicBlock::Create(ctx, "drop.yes", fn);
                    auto* no = BasicBlock::Create(ctx, "drop.no", fn);
                    builder.CreateCondBr(flag, yes, no);
                    gen.SetInsertPoint(yes);
                    emit_destroy_type(gen, type, ptr, coro.decl->encoded_location());
                    if(!gen.has_current_block_ended) {
                        builder.CreateBr(no);
                    }
                    gen.SetInsertPoint(no);
                } else {
                    emit_destroy_type(gen, type, ptr, coro.decl->encoded_location());
                }
            }
            // cancel the child future this site is waiting on (if any)
            if(site < layout.child_field.size() && layout.child_field[site] != 0 && layout.child_handle_ty[site] != nullptr) {
                auto* child = wrapper_field(layout.child_field[site]);
                auto* child_frame = builder.CreateLoad(ptr_ty, gep_idx(builder, layout.child_handle_ty[site], child, {0, 0}));
                auto* child_vtbl = builder.CreateLoad(ptr_ty, gep_idx(builder, layout.child_handle_ty[site], child, {0, 1}));
                auto* is_null = builder.CreateICmpEQ(child_frame, ConstantPointerNull::get(ptr_ty));
                auto* drop_child_bb = BasicBlock::Create(ctx, "drop.child", fn);
                auto* after_child_bb = BasicBlock::Create(ctx, "drop.child.after", fn);
                builder.CreateCondBr(is_null, after_child_bb, drop_child_bb);
                gen.SetInsertPoint(drop_child_bb);
                auto* child_drop_fn = builder.CreateLoad(ptr_ty, gep_idx(builder, ptr_ty, child_vtbl, {1}));
                builder.CreateCall(llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty}, false), child_drop_fn, {child_frame});
                builder.CreateStore(ConstantPointerNull::get(ptr_ty), gep_idx(builder, layout.child_handle_ty[site], child, {0, 0}));
                builder.CreateBr(after_child_bb);
                gen.SetInsertPoint(after_child_bb);
            }
            if(!gen.has_current_block_ended) {
                builder.CreateBr(done_bb);
            }
        }
    }

    gen.SetInsertPoint(done_bb);
    // The per-state cleanup has already run above; `coro.destroy` now only frees
    // the LLVM coroutine frame (it must not free our wrapper). Keep the pointers
    // in volatile slots because the split resume/destroy functions use `fastcc`
    // and may clobber callee-saved registers this C-convention function relies on.
    auto* wrapper_saved = builder.CreateAlloca(ptr_ty);
    builder.CreateStore(wrapper, wrapper_saved, true);
    auto* coro_frame = builder.CreateLoad(ptr_ty, wrapper_field(0));
    auto* coro_saved = builder.CreateAlloca(ptr_ty);
    builder.CreateStore(coro_frame, coro_saved, true);
    builder.CreateCall(destroy_fn, {coro_frame});
    auto* coro_reloaded = builder.CreateLoad(ptr_ty, coro_saved, true);
    auto* wrapper_reloaded = builder.CreateLoad(ptr_ty, wrapper_saved, true);
    builder.CreateCall(frame_free_fn, {coro_reloaded, ConstantInt::get(i64, 0), ConstantInt::get(i64, 0)});
    builder.CreateCall(frame_free_fn, {wrapper_reloaded, ConstantInt::get(i64, 0), ConstantInt::get(i64, 0)});
    builder.CreateRetVoid();

    // Completed coroutine: free both frames directly, without `coro.destroy`.
    gen.SetInsertPoint(free_bb);
    auto* wrapper_saved_free = builder.CreateAlloca(ptr_ty);
    builder.CreateStore(wrapper, wrapper_saved_free, true);
    auto* coro_frame_free = builder.CreateLoad(ptr_ty, wrapper_field(0));
    auto* wrapper_reloaded_free = builder.CreateLoad(ptr_ty, wrapper_saved_free, true);
    builder.CreateCall(frame_free_fn, {coro_frame_free, ConstantInt::get(i64, 0), ConstantInt::get(i64, 0)});
    builder.CreateCall(frame_free_fn, {wrapper_reloaded_free, ConstantInt::get(i64, 0), ConstantInt::get(i64, 0)});
    builder.CreateRetVoid();

    gen.current_function = prev_func;
    return fn;
}

/**
 * Builds the `FutureTable<T>` vtable global (field 0 = poll, field 1 = drop).
 */
static llvm::Value* emit_vtable(
        Codegen& gen,
        LLVMCoroContext& coro,
        llvm::Type* table_ty,
        llvm::Function* poll_fn,
        llvm::Function* drop_fn
) {
    auto* init = ConstantStruct::get(cast<llvm::StructType>(table_ty), {poll_fn, drop_fn});
    const auto name = coro.ramp->getName().str() + "__vtbl";
    return new GlobalVariable(*gen.module, table_ty, true, GlobalValue::InternalLinkage, init, name);
}

/**
 * Eager frame for an async function with no awaits (design Section 9.7): a
 * tiny heap frame holding only the result, an always-`Ready` `poll`, a `drop`
 * that frees the frame, and a ramp that allocates the frame, runs the body and
 * returns the handle. No coroutine intrinsics are involved.
 */
static bool gen_llvm_async_eager_fn(
        Codegen& gen,
        FunctionDeclaration* decl,
        BaseType* inner,
        BaseType* rt,
        llvm::Function* ramp,
        llvm::Type* table_ty,
        llvm::Type* poll_ty
) {
    auto& ctx = *gen.ctx;
    auto& builder = *gen.builder;
    auto* ptr_ty = builder.getPtrTy();
    auto* i32 = builder.getInt32Ty();
    auto* i64 = builder.getInt64Ty();
    auto* handle_ty = rt->llvm_type(gen);
    auto* inner_ty = inner->llvm_type(gen);
    const bool has_value = !inner_ty->isVoidTy() && !inner_ty->isEmptyTy();

    // frame { i32 state, T result }
    auto* frame_ty = llvm::StructType::create(ctx, {i32, inner_ty}, ramp->getName().str() + ".frame");
    const unsigned frame_align = 8;
    const auto frame_size = gen.module->getDataLayout().getTypeAllocSize(frame_ty);

    auto* alloc_ty = llvm::FunctionType::get(ptr_ty, {i64, i64}, false);
    auto* frame_alloc_fn = gen.module->getFunction("chemical_async_frame_alloc");
    if(frame_alloc_fn == nullptr) {
        frame_alloc_fn = Function::Create(alloc_ty, GlobalValue::ExternalLinkage, "chemical_async_frame_alloc", gen.module.get());
    }
    auto* free_ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty, i64, i64}, false);
    auto* frame_free_fn = gen.module->getFunction("chemical_async_frame_free");
    if(frame_free_fn == nullptr) {
        frame_free_fn = Function::Create(free_ty, GlobalValue::ExternalLinkage, "chemical_async_frame_free", gen.module.get());
    }

    // poll: `Poll.Ready(frame->result)`
    auto* poll_fn_ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty, ptr_ty, ptr_ty}, false);
    auto* poll_fn = Function::Create(poll_fn_ty, GlobalValue::InternalLinkage, ramp->getName().str() + "__poll", gen.module.get());
    {
        auto* entry = BasicBlock::Create(ctx, "entry", poll_fn);
        gen.SetInsertPoint(entry);
        auto* sret = poll_fn->getArg(0);
        auto* frame = poll_fn->getArg(1);
        llvm::Value* result = nullptr;
        if(has_value) {
            result = builder.CreateLoad(inner_ty, gep_idx(builder, frame_ty, frame, {0, 1}));
        }
        emit_poll(gen, sret, poll_ty, true, result);
        builder.CreateRetVoid();
    }

    // drop: free the frame
    auto* drop_ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty}, false);
    auto* drop_fn = Function::Create(drop_ty, GlobalValue::InternalLinkage, ramp->getName().str() + "__drop", gen.module.get());
    {
        auto* entry = BasicBlock::Create(ctx, "entry", drop_fn);
        gen.SetInsertPoint(entry);
        builder.CreateCall(frame_free_fn, {drop_fn->getArg(0), ConstantInt::get(i64, frame_size), ConstantInt::get(i64, frame_align)});
        builder.CreateRetVoid();
    }

    LLVMCoroContext coro;
    coro.decl = decl;
    coro.ramp = ramp;
    auto* vtbl = emit_vtable(gen, coro, table_ty, poll_fn, drop_fn);

    // ramp
    const auto prev_coro = gen.current_coro;
    const auto prev_func = gen.current_function;
    const auto prev_func_type = gen.current_func_type;
    gen.current_function = ramp;
    gen.SetInsertPoint(&ramp->getEntryBlock());
    auto* frame = builder.CreateCall(frame_alloc_fn, {ConstantInt::get(i64, frame_size), ConstantInt::get(i64, frame_align)});
    auto* sret = ramp->getArg(decl->getStructReturnArgIndex());
    builder.CreateStore(frame, gep_idx(builder, handle_ty, sret, {0, 0}));
    builder.CreateStore(vtbl, gep_idx(builder, handle_ty, sret, {0, 1}));

    coro.promise_ty = frame_ty;
    coro.promise = frame;
    coro.result_field = 1;
    coro.inner = inner;
    coro.inner_ty = inner_ty;
    coro.poll_ty = poll_ty;
    coro.handle_ty = handle_ty;
    auto* done_bb = BasicBlock::Create(ctx, "eager.done", ramp);
    auto* state_alloca = builder.CreateAlloca(i32);
    coro.state_ptr = state_alloca;
    coro.final_suspend_bb = done_bb;
    gen.current_coro = &coro;
    gen.current_func_type = decl;
    decl->queue_destruct_params(gen);
    for(auto& param : decl->params) {
        param->code_gen(gen);
    }
    gen.evaluated_func_calls.clear();
    decl->body.value().code_gen_no_scope(gen, 0);
    if(!gen.has_current_block_ended) {
        builder.CreateBr(done_bb);
    }
    gen.SetInsertPoint(done_bb);
    builder.CreateRetVoid();

    gen.current_coro = prev_coro;
    gen.current_function = prev_func;
    gen.current_func_type = prev_func_type;
    return true;
}

/**
 * Emits the synchronous C entry point for an application whose `main` is
 * `async func main`. The coroutine ramp returns a `FutureHandle<T>` through a
 * hidden sret pointer; this wrapper drives it with the same "no waker" context
 * `block_on` uses, drops the handle, and returns the integer result (design
 * Section 5.5). Mirrors the C backend's `emit_async_main_wrapper`.
 *
 * The caller must have renamed the ramp away from `main` (see
 * `gen_llvm_async_fn`) so this wrapper can own the real symbol.
 */
static void emit_llvm_async_main_wrapper(
        Codegen& gen,
        FunctionDeclaration* decl,
        BaseType* inner,
        BaseType* rt,
        llvm::Function* ramp,
        llvm::Type* poll_ty
) {
    auto& ctx = *gen.ctx;
    auto& builder = *gen.builder;
    auto* ptr_ty = builder.getPtrTy();
    auto* i32 = builder.getInt32Ty();
    auto* handle_ty = rt->llvm_type(gen);
    auto* inner_ty = inner->llvm_type(gen);
    const bool has_value = !inner_ty->isVoidTy() && !inner_ty->isEmptyTy();

    // entry signature mirrors the declared params, minus the hidden sret slot
    auto* ramp_ty = ramp->getFunctionType();
    const auto sret_index = decl->getStructReturnArgIndex();
    std::vector<llvm::Type*> entry_params;
    for(unsigned i = 0; i < ramp_ty->getNumParams(); i++) {
        if(i == (unsigned) sret_index) {
            continue;
        }
        entry_params.push_back(ramp_ty->getParamType(i));
    }
    auto* main_ty = llvm::FunctionType::get(i32, entry_params, false);
    auto* main_fn = gen.module->getFunction("main");
    if(main_fn != nullptr && main_fn->isDeclaration() && main_fn->use_empty()) {
        // drop a stale external declaration so the entry point has the right type
        main_fn->eraseFromParent();
        main_fn = nullptr;
    }
    if(main_fn == nullptr) {
        main_fn = llvm::Function::Create(main_ty, GlobalValue::ExternalLinkage, "main", gen.module.get());
    } else {
        main_fn->setLinkage(GlobalValue::ExternalLinkage);
    }

    const auto prev_func = gen.current_function;
    const auto prev_coro = gen.current_coro;
    const auto prev_func_type = gen.current_func_type;
    gen.current_function = main_fn;
    gen.current_coro = nullptr;
    gen.current_func_type = nullptr;

    auto* entry = BasicBlock::Create(ctx, "entry", main_fn);
    gen.SetInsertPoint(entry);
    auto* handle = builder.CreateAlloca(handle_ty);
    std::vector<llvm::Value*> ramp_args { handle };
    for(unsigned i = 0; i < entry_params.size(); i++) {
        ramp_args.push_back(main_fn->getArg(i));
    }
    builder.CreateCall(ramp_ty, ramp, ramp_args);

    auto* poll_tmp = builder.CreateAlloca(poll_ty);
    auto* null_ptr = ConstantPointerNull::get(ptr_ty);
    auto* loop_bb = BasicBlock::Create(ctx, "main.loop", main_fn);
    auto* done_bb = BasicBlock::Create(ctx, "main.done", main_fn);
    builder.CreateBr(loop_bb);

    // poll until Ready, then drop the handle (cancels a suspended frame once)
    gen.SetInsertPoint(loop_bb);
    auto* frame = builder.CreateLoad(ptr_ty, gep_idx(builder, handle_ty, handle, {0, 0}));
    auto* vtbl = builder.CreateLoad(ptr_ty, gep_idx(builder, handle_ty, handle, {0, 1}));
    auto* poll_fn = builder.CreateLoad(ptr_ty, vtbl);
    builder.CreateCall(llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty, ptr_ty, ptr_ty}, false), poll_fn, {poll_tmp, frame, null_ptr});
    auto* tag = builder.CreateLoad(i32, gep_idx(builder, poll_ty, poll_tmp, {0, 0}));
    auto* is_ready = builder.CreateICmpEQ(tag, builder.getInt32(0));
    builder.CreateCondBr(is_ready, done_bb, loop_bb);

    gen.SetInsertPoint(done_bb);
    llvm::Value* result = nullptr;
    if(has_value && inner_ty->isIntegerTy()) {
        auto* payload = builder.CreateLoad(inner_ty, gep_idx(builder, poll_ty, poll_tmp, {0, 1}));
        result = builder.CreateIntCast(payload, i32, true);
    }
    auto* frame2 = builder.CreateLoad(ptr_ty, gep_idx(builder, handle_ty, handle, {0, 0}));
    auto* vtbl2 = builder.CreateLoad(ptr_ty, gep_idx(builder, handle_ty, handle, {0, 1}));
    auto* drop_fn = builder.CreateLoad(ptr_ty, gep_idx(builder, ptr_ty, vtbl2, {1}));
    builder.CreateCall(llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty}, false), drop_fn, {frame2});
    builder.CreateRet(result != nullptr ? result : builder.getInt32(0));

    gen.current_function = prev_func;
    gen.current_coro = prev_coro;
    gen.current_func_type = prev_func_type;
}

bool gen_llvm_async_fn(Codegen& gen, FunctionDeclaration* decl) {
    BaseType* inner = nullptr;
    auto* rt = const_cast<BaseType*>(decl->returnType.getType());
    if(!decl->is_async() || !is_future_handle_type(rt, inner) || !decl->body.has_value()) {
        return false;
    }

    auto& ctx = *gen.ctx;
    auto& builder = *gen.builder;
    auto* ptr_ty = builder.getPtrTy();
    auto* i32 = builder.getInt32Ty();
    auto* i64 = builder.getInt64Ty();
    auto* i8 = builder.getInt8Ty();
    auto* i1 = builder.getInt1Ty();

    auto* ramp = decl->llvm_func(gen);
    if(ramp == nullptr) {
        return false;
    }
    // Capture the caller's current function *before* claiming the coroutine
    // ramp: the async lowering runs at module scope, and every exit path below
    // must put the caller's (null) function back. The previous code restored
    // `prev_func` captured *after* the assignment below, so it restored `ramp`
    // itself and left `current_function` non-null. Module-level variable
    // initializers that followed an `async func` were then compiled as locals
    // and their LLVM global was emitted without an initializer (`internal global
    // i32`), producing invalid IR that crashed a later pass (B27).
    const auto caller_function = gen.current_function;
    // An application's `async func main` needs a synchronous `int main` entry
    // point. Move the coroutine ramp out of the way so the wrapper (emitted at
    // the end) can own the `main` symbol.
    const bool is_async_main = decl->is_no_mangle() && decl->name_view() == chem::string_view("main");
    if(is_async_main) {
        ramp->setName("__chx_async_main");
    }
    gen.current_function = ramp;
    auto* handle_ty = rt->llvm_type(gen);
    auto* inner_ty = inner->llvm_type(gen);

    // Resolve `Poll<T>` / `FutureTable<T>` structurally from the handle's vtbl.
    BaseType* table_bt = nullptr;
    if(rt->get_direct_linked_container() != nullptr) {
        auto* fh = rt->get_direct_linked_container();
        for(const auto member : fh->variables()) {
            if(member->name != chem::string_view("vtbl")) {
                continue;
            }
            auto* vt = const_cast<BaseType*>(member->as_struct_member_unsafe()->type.getType());
            if(vt->kind() == BaseTypeKind::Pointer) {
                table_bt = const_cast<BaseType*>(vt->as_pointer_type_unsafe()->type);
            } else if(vt->kind() == BaseTypeKind::Reference) {
                table_bt = const_cast<BaseType*>(vt->as_reference_type_unsafe()->type);
            }
        }
    }
    BaseType* poll_bt = nullptr;
    if(table_bt != nullptr && table_bt->get_direct_linked_container() != nullptr) {
        auto* table = table_bt->get_direct_linked_container();
        for(const auto member : table->variables()) {
            if(member->name != chem::string_view("poll")) {
                continue;
            }
            auto* ft = const_cast<BaseType*>(member->as_struct_member_unsafe()->type.getType());
            if(ft->kind() == BaseTypeKind::Function) {
                poll_bt = const_cast<BaseType*>(ft->as_function_type_unsafe()->returnType.getType());
            }
        }
    }
    if(table_bt == nullptr || poll_bt == nullptr) {
        gen.current_function = caller_function;
        return false;
    }
    auto* table_ty = table_bt->llvm_type(gen);
    auto* poll_ty = poll_bt->llvm_type(gen);

    // the shared lowering plan (suspension sites + frame-backed slots), used for
    // the eager check, the explicit frame layout, and the cancel cleanup
    auto plan = build_async_plan(decl);

    // an async function that never awaits has no suspension points: use the
    // eager frame instead of a coroutine (design Section 9.7)
    if(plan.sites.empty()) {
        const auto ok = gen_llvm_async_eager_fn(gen, decl, inner, rt, ramp, table_ty, poll_ty);
        // the eager helper restores to its own `prev_func` (the ramp); put the
        // caller's function back instead (B27)
        gen.current_function = caller_function;
        if(ok && is_async_main) {
            emit_llvm_async_main_wrapper(gen, decl, inner, rt, ramp, poll_ty);
        }
        return ok;
    }

    // the ramp must carry the `presplitcoroutine` enum attribute or the LLVM
    // coroutine passes will not transform it (design Section 9.0 item 1). Using
    // the string form adds a *string* attribute that CoroSplit does not match.
    ramp->addFnAttr(llvm::Attribute::PresplitCoroutine);

    // ---- explicit frame layout ----
    // Every local in scope at a suspension point is stored in the compiler-owned
    // wrapper frame (`{ coro, state, cx, result, <slots>, <flags>, <children> }`)
    // rather than as a body alloca promoted by CoroFrame. This mirrors the 2c
    // backend's `__chx_slot_<id>` / `__chx_drop_<id>` / `__chx_child_<i>` fields
    // and is what makes cancellation cleanup safe (design Section 8.1/8.3/8.5):
    // the destroy path reads only stable, non-aliased fields.
    AsyncFrameLayout layout;
    layout.plan = &plan;
    layout.slot_field.assign(plan.slots.size(), 0);
    layout.flag_field.assign(plan.slots.size(), 0);
    layout.child_field.assign(plan.sites.size(), 0);
    layout.child_handle_ty.assign(plan.sites.size(), nullptr);

    std::vector<bool> crosses(plan.slots.size(), false);
    for(auto& site : plan.sites) {
        for(auto id : site.live_slots) {
            if(id < crosses.size()) {
                crosses[id] = true;
            }
        }
    }

    std::vector<llvm::Type*> frame_fields { ptr_ty, i32, ptr_ty, inner_ty };
    unsigned next_field = 4;
    for(size_t id = 0; id < plan.slots.size(); id++) {
        const auto& slot = plan.slots[id];
        if(slot.node == nullptr || slot.type == nullptr || !crosses[id]) {
            continue;
        }
        // only locals are pinned; parameters are stable SSA args / spill slots
        if(slot.node->kind() != ASTNodeKind::VarInitStmt) {
            continue;
        }
        auto* slot_llvm = slot.type->llvm_type(gen);
        if(slot_llvm == nullptr) {
            continue;
        }
        frame_fields.push_back(slot_llvm);
        layout.slot_field[id] = next_field++;
        if(slot.destructible) {
            frame_fields.push_back(builder.getInt1Ty());
            layout.flag_field[id] = next_field++;
        }
    }
    for(size_t i = 0; i < plan.sites.size(); i++) {
        auto* handle_bt = plan.sites[i].awaited_handle_type;
        if(handle_bt == nullptr) {
            continue;
        }
        auto* handle_llvm = handle_bt->llvm_type(gen);
        if(handle_llvm == nullptr) {
            continue;
        }
        frame_fields.push_back(handle_llvm);
        layout.child_field[i] = next_field++;
        layout.child_handle_ty[i] = handle_llvm;
    }

    // The value in the returned handle's `frame` field is our own wrapper
    // `{ coro, state, cx, result, ... }`; the LLVM coroutine frame is a *separate*
    // allocation referenced by `coro`. This gives `poll`/`drop` fixed field
    // offsets instead of relying on `coro.promise`, whose slot CoroFrame may place
    // after large spills.
    auto* promise_ty = llvm::StructType::create(ctx, frame_fields, ramp->getName().str() + ".frame");
    const unsigned promise_align = 8;
    // the promise always sits right after the frame's resume/destroy pointers
    const unsigned promise_offset = 2 * gen.module->getDataLayout().getPointerSize();

    auto* id_fn = coro_intrinsic(gen, Intrinsic::coro_id);
    auto* alloc_fn = coro_intrinsic(gen, Intrinsic::coro_alloc);
    auto* size_fn = coro_intrinsic(gen, Intrinsic::coro_size, {i64});
    auto* begin_fn = coro_intrinsic(gen, Intrinsic::coro_begin);
    auto* save_fn = coro_intrinsic(gen, Intrinsic::coro_save);
    auto* suspend_fn = coro_intrinsic(gen, Intrinsic::coro_suspend);
    auto* resume_fn = coro_intrinsic(gen, Intrinsic::coro_resume);
    auto* destroy_fn = coro_intrinsic(gen, Intrinsic::coro_destroy);
    auto* done_fn = coro_intrinsic(gen, Intrinsic::coro_done);
    auto* promise_fn = coro_intrinsic(gen, Intrinsic::coro_promise);
    auto* free_fn = coro_intrinsic(gen, Intrinsic::coro_free);
    auto* end_fn = coro_intrinsic(gen, Intrinsic::coro_end);

    auto* alloc_ty = llvm::FunctionType::get(ptr_ty, {i64, i64}, false);
    auto* frame_alloc_fn = gen.module->getFunction("chemical_async_frame_alloc");
    if(frame_alloc_fn == nullptr) {
        frame_alloc_fn = Function::Create(alloc_ty, GlobalValue::ExternalLinkage,
                                          "chemical_async_frame_alloc", gen.module.get());
    }
    auto* free_ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty, i64, i64}, false);
    auto* frame_free_fn = gen.module->getFunction("chemical_async_frame_free");
    if(frame_free_fn == nullptr) {
        frame_free_fn = Function::Create(free_ty, GlobalValue::ExternalLinkage,
                                         "chemical_async_frame_free", gen.module.get());
    }

    LLVMCoroContext coro;
    coro.decl = decl;
    coro.ramp = ramp;
    coro.promise_ty = promise_ty;
    coro.promise_align = promise_align;
    coro.promise_offset = promise_offset;
    coro.result_field = 3;
    coro.cx_field = 2;
    coro.state_field = 1;
    coro.inner = inner;
    coro.inner_ty = inner_ty;
    coro.poll_ty = poll_ty;
    coro.handle_ty = handle_ty;
    coro.frame_align = ConstantInt::get(i64, promise_align);
    coro.frame_layout = &layout;

    const auto prev_coro = gen.current_coro;
    const auto prev_func_type = gen.current_func_type;
    const auto prev_redirect = gen.redirect_return;
    gen.current_coro = &coro;

    auto* poll_fn = emit_poll_fn(gen, coro, resume_fn, done_fn);
    auto* drop_fn = emit_drop_fn(gen, coro, layout, destroy_fn, frame_free_fn);
    auto* vtbl = emit_vtable(gen, coro, table_ty, poll_fn, drop_fn);

    // ---- ramp ----
    auto* entry = &ramp->getEntryBlock();
    gen.SetInsertPoint(entry);
    coro.handle_sret = ramp->getArg(decl->getStructReturnArgIndex());
    // pointer to our wrapper, kept in the entry block so CoroFrame spills it
    // across suspends for the body's `return` to find
    auto* wrap_alloca = builder.CreateAlloca(ptr_ty);

    auto* null_ptr = ConstantPointerNull::get(ptr_ty);
    auto* id = builder.CreateCall(id_fn, {ConstantInt::get(i32, 0), null_ptr, null_ptr, null_ptr});
    coro.id = id;
    auto* must_alloc = builder.CreateCall(alloc_fn, {id});
    auto* alloc_bb = BasicBlock::Create(ctx, "coro.alloc", ramp);
    auto* after_bb = BasicBlock::Create(ctx, "coro.afteralloc", ramp);
    builder.CreateCondBr(must_alloc, alloc_bb, after_bb);

    gen.SetInsertPoint(alloc_bb);
    auto* size = builder.CreateCall(size_fn);
    auto* mem = builder.CreateCall(frame_alloc_fn, {size, ConstantInt::get(i64, promise_align)});
    builder.CreateBr(after_bb);

    gen.SetInsertPoint(after_bb);
    auto* frame = builder.CreatePHI(ptr_ty, 2);
    frame->addIncoming(mem, alloc_bb);
    frame->addIncoming(null_ptr, entry);
    auto* handle = builder.CreateCall(begin_fn, {id, frame});
    coro.frame = handle;
    coro.frame_size = size;

    // allocate our wrapper, point it at the coroutine frame and initialise its
    // state/cx fields
    const auto wrapper_size = gen.module->getDataLayout().getTypeAllocSize(promise_ty);
    auto* wrapper = builder.CreateCall(frame_alloc_fn, {ConstantInt::get(i64, wrapper_size), ConstantInt::get(i64, promise_align)});
    builder.CreateStore(handle, gep_idx(builder, promise_ty, wrapper, {0, 0}));
    coro.state_ptr = gep_idx(builder, promise_ty, wrapper, {0, 1});
    builder.CreateStore(ConstantInt::get(i32, 0), coro.state_ptr);
    builder.CreateStore(null_ptr, gep_idx(builder, promise_ty, wrapper, {0, (unsigned) coro.cx_field}));
    builder.CreateStore(wrapper, wrap_alloca);
    coro.promise = builder.CreateLoad(ptr_ty, wrap_alloca);

    // Pre-seed each pinned local's frame field (and drop flag) so
    // `VarInitStatement::code_gen` writes into it instead of a body alloca. The
    // GEPs are emitted once here in the ramp setup and dominate the body.
    for(size_t id = 0; id < plan.slots.size(); id++) {
        if(layout.slot_field[id] == 0) {
            continue;
        }
        const auto& slot = plan.slots[id];
        auto* field_ptr = gep_idx(builder, promise_ty, wrapper, {0, layout.slot_field[id]});
        gen.pinned_slots[slot.node] = field_ptr;
        if(slot.node->kind() == ASTNodeKind::VarInitStmt) {
            slot.node->as_var_init_unsafe()->llvm_ptr = field_ptr;
        }
        if(layout.flag_field[id] != 0) {
            auto* flag_ptr = gep_idx(builder, promise_ty, wrapper, {0, layout.flag_field[id]});
            gen.pinned_drop_flags[slot.node] = flag_ptr;
        }
    }

    // the returned handle's frame is our wrapper
    auto* frame_field = gep_idx(builder, handle_ty, coro.handle_sret, {0, 0});
    builder.CreateStore(wrapper, frame_field);
    auto* vtbl_field = gep_idx(builder, handle_ty, coro.handle_sret, {0, 1});
    builder.CreateStore(vtbl, vtbl_field);

    auto* body_bb = BasicBlock::Create(ctx, "coro.body", ramp);
    coro.cleanup_bb = BasicBlock::Create(ctx, "coro.cleanup", ramp);
    // the ramp's return-to-caller path (and the normal-completion path goes
    // through here too); it is the default of both suspends so exactly one
    // `coro.end(..., i1 false, ...)` exists (design Section 9.0 item 5)
    auto* ramp_ret_bb = BasicBlock::Create(ctx, "coro.suspend", ramp);
    coro.ramp_ret_bb = ramp_ret_bb;
    auto* save_init = builder.CreateCall(save_fn, {handle});
    auto* s_init = builder.CreateCall(suspend_fn, {save_init, ConstantInt::get(i1, false)});
    auto* init_switch = builder.CreateSwitch(s_init, ramp_ret_bb, 2);
    init_switch->addCase(ConstantInt::get(i8, 0), body_bb);
    init_switch->addCase(ConstantInt::get(i8, 1), coro.cleanup_bb);

    // ---- body ----
    gen.SetInsertPoint(body_bb);
    coro.final_suspend_bb = BasicBlock::Create(ctx, "coro.final", ramp);
    gen.redirect_return = coro.final_suspend_bb;
    {
        const auto prev_ft = gen.current_func_type;
        gen.current_func_type = decl;
        decl->queue_destruct_params(gen);
        for(auto& param : decl->params) {
            param->code_gen(gen);
        }
        gen.evaluated_func_calls.clear();
        decl->body.value().code_gen_no_scope(gen, 0);
        gen.current_func_type = prev_ft;
    }
    if(!gen.has_current_block_ended) {
        builder.CreateBr(coro.final_suspend_bb);
    }

    gen.SetInsertPoint(coro.final_suspend_bb);
    // mark the frame DONE so `drop` runs no cancellation cleanup
    builder.CreateStore(ConstantInt::get(i32, 0xFFFFFFFFu), coro.state_ptr);
    auto* save_f = builder.CreateCall(save_fn, {handle});
    auto* s_f = builder.CreateCall(suspend_fn, {save_f, ConstantInt::get(i1, true)});
    auto* done_bb = BasicBlock::Create(ctx, "coro.done", ramp);
    auto* fin_switch = builder.CreateSwitch(s_f, done_bb, 2);
    fin_switch->addCase(ConstantInt::get(i8, 0), done_bb);
    fin_switch->addCase(ConstantInt::get(i8, 1), coro.cleanup_bb);

    // Normal completion: the frame stays alive so `drop` (coro.destroy) can free
    // it exactly once. Freeing here too would double-free when the returned
    // handle is destroyed (design Section 9.5).
    gen.SetInsertPoint(done_bb);
    builder.CreateBr(ramp_ret_bb);

    // the single normal `coro.end(..., i1 false, ...)`
    gen.SetInsertPoint(ramp_ret_bb);
    builder.CreateCall(end_fn, {handle, ConstantInt::get(i1, false), ConstantTokenNone::get(ctx)});
    builder.CreateRetVoid();

    // Destroy-while-suspended: `coro.end(i1 true)` only. The frame is freed by
    // `drop` after `coro.destroy` returns — CoroSplit's `coro.end` lowering
    // writes the resume/destroy pointers into the frame, so freeing here is a UAF.
    gen.SetInsertPoint(coro.cleanup_bb);
    builder.CreateCall(end_fn, {handle, ConstantInt::get(i1, true), ConstantTokenNone::get(ctx)});
    builder.CreateRetVoid();

    gen.current_coro = prev_coro;
    // `prev_func` was captured after the ramp was installed, so it is the ramp;
    // restore the function the caller was compiling (module scope -> nullptr) (B27)
    gen.current_function = caller_function;
    gen.current_func_type = prev_func_type;
    gen.redirect_return = prev_redirect;
    gen.pinned_slots.clear();
    gen.pinned_drop_flags.clear();
    if(is_async_main) {
        emit_llvm_async_main_wrapper(gen, decl, inner, rt, ramp, poll_ty);
    }
    return true;
}

llvm::Value* gen_llvm_await(Codegen& gen, AwaitExpression* await) {
    auto* coro = gen.current_coro;
    if(coro == nullptr || await->getInner() == nullptr) {
        return nullptr;
    }
    auto& ctx = *gen.ctx;
    auto& builder = *gen.builder;
    auto* ptr_ty = builder.getPtrTy();
    auto* i32 = builder.getInt32Ty();
    auto* i8 = builder.getInt8Ty();

    auto* save_fn = coro_intrinsic(gen, Intrinsic::coro_save);
    auto* suspend_fn = coro_intrinsic(gen, Intrinsic::coro_suspend);

    // The awaited future's result type may differ from the enclosing function's
    // (e.g. `async func use_label() : int` awaiting `make_label() : std::string`).
    // All `FutureHandle<T>` share the same `{ frame, vtbl }` layout, but the
    // `Poll<T>` result and payload are the *child's* types, not the enclosing
    // coroutine's. Using the enclosing types here emitted a `Poll<int>` for a
    // `std::string` future, which crashed the backend (memcpy from an i32).
    auto* child_rt = const_cast<BaseType*>(await->getInner()->getType());
    BaseType* child_inner = nullptr;
    llvm::Type* child_handle_ty = coro->handle_ty;
    llvm::Type* child_poll_ty = coro->poll_ty;
    llvm::Type* child_inner_ty = coro->inner_ty;
    if(child_rt != nullptr && is_future_handle_type(child_rt, child_inner)) {
        child_handle_ty = child_rt->llvm_type(gen);
        child_inner_ty = child_inner->llvm_type(gen);
        if(auto* resolved = resolve_poll_type_from_handle(gen, child_rt)) {
            child_poll_ty = resolved;
        }
    }

    auto* child_ptr = await->getInner()->llvm_pointer(gen);
    if(child_ptr == nullptr) {
        return nullptr;
    }

    // Locate this await's suspension site (its hoisted `var x = await e`) so the
    // child future is stored in the frame for cancellation and the wrapper state
    // can mark where the coroutine is suspended.
    const auto* layout = (const AsyncFrameLayout*) coro->frame_layout;
    unsigned site_index = UINT_MAX;
    if(layout != nullptr && layout->plan != nullptr) {
        for(unsigned i = 0; i < layout->plan->sites.size(); i++) {
            auto* vi = layout->plan->sites[i].var_init;
            if(vi != nullptr && vi->kind() == ASTNodeKind::VarInitStmt
               && vi->as_var_init_unsafe()->value == await) {
                site_index = i;
                break;
            }
        }
    }

    auto* child_frame = builder.CreateLoad(ptr_ty, gep_idx(builder, child_handle_ty, child_ptr, {0, 0}));
    auto* child_vtbl = builder.CreateLoad(ptr_ty, gep_idx(builder, child_handle_ty, child_ptr, {0, 1}));
    auto* poll_fn = builder.CreateLoad(ptr_ty, child_vtbl);
    auto* child_drop_fn = builder.CreateLoad(ptr_ty, gep_idx(builder, ptr_ty, child_vtbl, {1}));
    auto* child_drop_ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty}, false);

    // Persist the child future handle in the frame so the cancel path (`drop`)
    // can drop it (the body alloca is not reachable from `drop`).
    if(site_index != UINT_MAX && layout->child_field[site_index] != 0) {
        auto* field = gep_idx(builder, coro->promise_ty, coro->promise, {0, layout->child_field[site_index]});
        const auto size = gen.module->getDataLayout().getTypeAllocSize(child_handle_ty);
        builder.CreateMemCpy(field, llvm::MaybeAlign(), child_ptr, llvm::MaybeAlign(), size);
    }

    // `await` consumes its operand: clear the source handle so the operand's own
    // destructor (at scope end / when a temporary is cleaned up) does not drop
    // the same frame a second time. We already captured `child_frame` above, so
    // the poll loop and the final `child_drop_fn` below are unaffected.
    {
        auto* null = ConstantPointerNull::get(ptr_ty);
        builder.CreateStore(null, gep_idx(builder, child_handle_ty, child_ptr, {0, 0}));
        builder.CreateStore(null, gep_idx(builder, child_handle_ty, child_ptr, {0, 1}));
    }

    auto* poll_tmp = builder.CreateAlloca(child_poll_ty);
    auto* loop_bb = BasicBlock::Create(ctx, "await.loop", gen.current_function);
    auto* suspend_bb = BasicBlock::Create(ctx, "await.suspend", gen.current_function);
    auto* done_bb = BasicBlock::Create(ctx, "await.done", gen.current_function);
    builder.CreateBr(loop_bb);

    gen.SetInsertPoint(loop_bb);
    // Reload the Context pointer from the wrapper on *every* iteration, including
    // after a resume. `emit_poll_fn` stores the caller's Context into the wrapper
    // at each poll entry; the caller (e.g. `executor_poll_tasks`) passes a fresh
    // stack Context every time. Caching it before the loop meant a resumed
    // coroutine forwarded a stale pointer to its child poll, so a combinator read
    // a garbage waker vtbl and jumped through it (B25).
    auto* cx = builder.CreateLoad(ptr_ty, gep_idx(builder, coro->promise_ty, coro->promise, {0, (unsigned) coro->cx_field}));
    auto* poll_fn_ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty, ptr_ty, ptr_ty}, false);
    builder.CreateCall(poll_fn_ty, poll_fn, {poll_tmp, child_frame, cx});
    auto* tag = builder.CreateLoad(i32, gep_idx(builder, child_poll_ty, poll_tmp, {0, 0}));
    auto* is_ready = builder.CreateICmpEQ(tag, builder.getInt32(0));
    builder.CreateCondBr(is_ready, done_bb, suspend_bb);

    gen.SetInsertPoint(suspend_bb);
    // mark the suspension site so `drop` selects the right live set
    if(site_index != UINT_MAX) {
        builder.CreateStore(ConstantInt::get(i32, site_index + 1), coro->state_ptr);
    }
    auto* save = builder.CreateCall(save_fn, {coro->frame});
    auto* s = builder.CreateCall(suspend_fn, {save, builder.getInt1(false)});
    // default (`-1`, first pass in the ramp) -> return to the caller; on resume
    // (`i8 0`) re-poll the child; `i8 1` -> destroy (handled by `drop`)
    auto* sw = builder.CreateSwitch(s, coro->ramp_ret_bb, 2);
    sw->addCase(ConstantInt::get(i8, 0), loop_bb);
    sw->addCase(ConstantInt::get(i8, 1), coro->cleanup_bb);

    gen.SetInsertPoint(done_bb);
    llvm::Value* result = nullptr;
    if(!child_inner_ty->isVoidTy()) {
        auto* payload = gep_idx(builder, child_poll_ty, poll_tmp, {0, 1});
        result = builder.CreateLoad(child_inner_ty, payload);
    }
    builder.CreateCall(child_drop_ty, child_drop_fn, {child_frame});
    return result;
}

#endif // COMPILER_BUILD
