// Copyright (c) Chemical Language Foundation 2026.

#ifdef COMPILER_BUILD

#include "LLVMCoroutine.h"

#include "compiler/Codegen.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/FunctionType.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/structures/Scope.h"
#include "ast/values/AwaitExpression.h"
#include "ast/base/Value.h"

#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <vector>

using namespace llvm;

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
        llvm::Function* promise_fn,
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
    auto* frame = fn->getArg(1);
    auto* cx = fn->getArg(2);

    auto* entry = BasicBlock::Create(ctx, "entry", fn);
    gen.SetInsertPoint(entry);

    // the same promise CoroFrame materialized for the ramp
    auto* promise = builder.CreateCall(promise_fn, {frame, builder.getInt32(coro.promise_align), builder.getInt1(false)});
    auto* cx_ptr = gep_idx(builder, coro.promise_ty, promise, {0, (unsigned) coro.cx_field});
    builder.CreateStore(cx, cx_ptr);

    builder.CreateCall(resume_fn, {frame});
    auto* done = builder.CreateCall(done_fn, {frame});
    auto* ready_bb = BasicBlock::Create(ctx, "ready", fn);
    auto* pending_bb = BasicBlock::Create(ctx, "pending", fn);
    builder.CreateCondBr(done, ready_bb, pending_bb);

    gen.SetInsertPoint(pending_bb);
    emit_poll(gen, sret, coro.poll_ty, false, nullptr);
    builder.CreateRetVoid();

    gen.SetInsertPoint(ready_bb);
    llvm::Value* result = nullptr;
    if(!coro.inner_ty->isVoidTy()) {
        auto* result_ptr = gep_idx(builder, coro.promise_ty, promise, {0, (unsigned) coro.result_field});
        result = builder.CreateLoad(coro.inner_ty, result_ptr);
    }
    emit_poll(gen, sret, coro.poll_ty, true, result);
    builder.CreateRetVoid();
    return fn;
}

/**
 * Emits `void @foo_drop(void* frame)`.
 */
static llvm::Function* emit_drop_fn(
        Codegen& gen,
        LLVMCoroContext& coro,
        llvm::Function* destroy_fn,
        llvm::Function* frame_free_fn
) {
    auto& ctx = *gen.ctx;
    auto& builder = *gen.builder;
    auto* ptr_ty = builder.getPtrTy();
    auto* i64 = builder.getInt64Ty();
    const auto name = coro.ramp->getName().str() + "__drop";
    auto* ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty}, false);
    auto* fn = Function::Create(ty, GlobalValue::InternalLinkage, name, gen.module.get());
    auto* entry = BasicBlock::Create(ctx, "entry", fn);
    gen.SetInsertPoint(entry);
    // `coro.destroy` runs the cleanup (which must not free the frame, because
    // CoroSplit's `coro.end` lowering writes the resume/destroy pointers into it
    // afterwards); free it here, once, after the destroy path has run.
    builder.CreateCall(destroy_fn, {fn->getArg(0)});
    builder.CreateCall(frame_free_fn, {fn->getArg(0), ConstantInt::get(i64, 0), ConstantInt::get(i64, 0)});
    builder.CreateRetVoid();
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
    // the ramp must carry the `presplitcoroutine` enum attribute or the LLVM
    // coroutine passes will not transform it (design Section 9.0 item 1). Using
    // the string form adds a *string* attribute that CoroSplit does not match.
    ramp->addFnAttr(llvm::Attribute::PresplitCoroutine);
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
        return false;
    }
    auto* table_ty = table_bt->llvm_type(gen);
    auto* poll_ty = poll_bt->llvm_type(gen);

    // promise struct { i32 state, ptr cx, T result }
    auto* promise_ty = llvm::StructType::create(ctx, {i32, ptr_ty, inner_ty}, ramp->getName().str() + ".promise");
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
    coro.inner = inner;
    coro.inner_ty = inner_ty;
    coro.poll_ty = poll_ty;
    coro.handle_ty = handle_ty;
    coro.frame_align = ConstantInt::get(i64, promise_align);

    const auto prev_coro = gen.current_coro;
    const auto prev_func = gen.current_function;
    const auto prev_func_type = gen.current_func_type;
    const auto prev_redirect = gen.redirect_return;
    gen.current_coro = &coro;

    auto* poll_fn = emit_poll_fn(gen, coro, promise_fn, resume_fn, done_fn);
    auto* drop_fn = emit_drop_fn(gen, coro, destroy_fn, frame_free_fn);
    auto* vtbl = emit_vtable(gen, coro, table_ty, poll_fn, drop_fn);

    // ---- ramp ----
    auto* entry = &ramp->getEntryBlock();
    gen.SetInsertPoint(entry);
    coro.handle_sret = ramp->getArg(decl->getStructReturnArgIndex());

    // Clang's promise pattern: an alloca of the promise type in the entry block
    // tells CoroFrame the promise size. The promise itself always sits right
    // after the frame's resume/destroy pointers (`frame + 2 * sizeof(void*)`), so
    // field access uses that fixed offset consistently in the ramp and in
    // `poll`/`drop` (design Section 9.3).
    // Clang's promise pattern: a plain alloca of the promise type inside the
    // coroutine is materialized into the frame by CoroFrame, and accessors
    // outside the coroutine recover its address with `coro.promise(frame, ...)`.
    auto* promise_alloca = builder.CreateAlloca(promise_ty);

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
    // inside the coroutine the promise is just the alloca; CoroFrame rewrites it
    // to the frame slot that `poll`/`drop` read via `coro.promise(frame, ...)`
    auto* promise = promise_alloca;
    coro.promise = promise;
    coro.state_ptr = gep_idx(builder, promise_ty, promise, {0, 0});
    auto* cx_ptr = gep_idx(builder, promise_ty, promise, {0, (unsigned) coro.cx_field});
    builder.CreateStore(ConstantInt::get(i32, 0), coro.state_ptr);
    builder.CreateStore(null_ptr, cx_ptr);

    auto* frame_field = gep_idx(builder, handle_ty, coro.handle_sret, {0, 0});
    builder.CreateStore(handle, frame_field);
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
    builder.CreateCall(end_fn, {null_ptr, ConstantInt::get(i1, false), ConstantTokenNone::get(ctx)});
    builder.CreateRetVoid();

    // Destroy-while-suspended: `coro.end(i1 true)` only. The frame is freed by
    // `drop` after `coro.destroy` returns — CoroSplit's `coro.end` lowering
    // writes the resume/destroy pointers into the frame, so freeing here is a UAF.
    gen.SetInsertPoint(coro.cleanup_bb);
    builder.CreateCall(end_fn, {null_ptr, ConstantInt::get(i1, true), ConstantTokenNone::get(ctx)});
    builder.CreateRetVoid();

    gen.current_coro = prev_coro;
    gen.current_function = prev_func;
    gen.current_func_type = prev_func_type;
    gen.redirect_return = prev_redirect;
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

    auto* child_ptr = await->getInner()->llvm_pointer(gen);
    if(child_ptr == nullptr) {
        return nullptr;
    }
    auto* child_frame = builder.CreateLoad(ptr_ty, gep_idx(builder, coro->handle_ty, child_ptr, {0, 0}));
    auto* child_vtbl = builder.CreateLoad(ptr_ty, gep_idx(builder, coro->handle_ty, child_ptr, {0, 1}));
    auto* poll_fn = builder.CreateLoad(ptr_ty, child_vtbl);
    auto* cx = builder.CreateLoad(ptr_ty, gep_idx(builder, coro->promise_ty, coro->promise, {0, (unsigned) coro->cx_field}));

    auto* poll_tmp = builder.CreateAlloca(coro->poll_ty);
    auto* loop_bb = BasicBlock::Create(ctx, "await.loop", gen.current_function);
    auto* suspend_bb = BasicBlock::Create(ctx, "await.suspend", gen.current_function);
    auto* done_bb = BasicBlock::Create(ctx, "await.done", gen.current_function);
    builder.CreateBr(loop_bb);

    gen.SetInsertPoint(loop_bb);
    auto* child_poll_ty = llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty, ptr_ty, ptr_ty}, false);
    builder.CreateCall(child_poll_ty, poll_fn, {poll_tmp, child_frame, cx});
    auto* tag = builder.CreateLoad(i32, gep_idx(builder, coro->poll_ty, poll_tmp, {0, 0}));
    auto* is_ready = builder.CreateICmpEQ(tag, builder.getInt32(0));
    builder.CreateCondBr(is_ready, done_bb, suspend_bb);

    gen.SetInsertPoint(suspend_bb);
    auto* save = builder.CreateCall(save_fn, {coro->frame});
    auto* s = builder.CreateCall(suspend_fn, {save, builder.getInt1(false)});
    // default (`-1`, first pass in the ramp) -> return to the caller; on resume
    // (`i8 0`) re-poll the child; `i8 1` -> destroy
    auto* sw = builder.CreateSwitch(s, coro->ramp_ret_bb, 2);
    sw->addCase(ConstantInt::get(i8, 0), loop_bb);
    sw->addCase(ConstantInt::get(i8, 1), coro->cleanup_bb);

    gen.SetInsertPoint(done_bb);
    llvm::Value* result = nullptr;
    if(!coro->inner_ty->isVoidTy()) {
        auto* payload = gep_idx(builder, coro->poll_ty, poll_tmp, {0, 1});
        result = builder.CreateLoad(coro->inner_ty, payload);
    }
    auto* child_drop_fn = builder.CreateLoad(ptr_ty, gep_idx(builder, ptr_ty, child_vtbl, {1}));
    builder.CreateCall(llvm::FunctionType::get(builder.getVoidTy(), {ptr_ty}, false), child_drop_fn, {child_frame});
    return result;
}

#endif // COMPILER_BUILD
