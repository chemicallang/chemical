// Copyright (c) Chemical Language Foundation 2025.

#include "ForLoop.h"
#include "ForInLoop.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/MembersContainer.h"
#include "compiler/symres/ImplementationsIndex.h"
#include <vector>

static llvm::Value* call_for_in_interface_fn(
        Codegen& gen,
        FunctionDeclaration* fn,
        std::vector<llvm::Value*> args,
        SourceLocation location,
        llvm::Value* struct_return_ptr = nullptr
) {
    if (fn->returnType->isStructLikeType()) {
        const auto retPtr = struct_return_ptr ? struct_return_ptr : gen.llvm.CreateAlloca(fn->returnType->llvm_type(gen), location);
        args.insert(args.begin(), retPtr);
        const auto callInstr = gen.builder->CreateCall(fn->llvm_func(gen), args);
        gen.di.instr(callInstr, location);
        return retPtr;
    }
    const auto callInstr = gen.builder->CreateCall(fn->llvm_func(gen), args);
    gen.di.instr(callInstr, location);
    return callInstr;
}

void ForLoop::code_gen(Codegen &gen) {

    // initialize the variables
    initializer->code_gen(gen);

    // creating blocks
    auto condBlock = llvm::BasicBlock::Create(*gen.ctx, "forcond", gen.current_function);
    auto thenBlock = llvm::BasicBlock::Create(*gen.ctx, "forthen", gen.current_function);
    auto thenIncrementBlock = llvm::BasicBlock::Create(*gen.ctx, "forinc", gen.current_function);
    auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "forend", gen.current_function);

    // going to condition
    gen.CreateBr(condBlock, body.encoded_location());

    // condition block
    gen.SetInsertPoint(condBlock);
    auto comparison = conditionExpr->llvm_value(gen);
    gen.CreateCondBr(comparison, thenBlock, endBlock, body.encoded_location());

    // then block
    gen.SetInsertPoint(thenBlock);
    gen.loop_body_gen(body, thenIncrementBlock, endBlock);
    gen.CreateBr(thenIncrementBlock, incrementerExpr->encoded_location());

    // increment block
    gen.SetInsertPoint(thenIncrementBlock);
    incrementerExpr->code_gen(gen);

    // TODO use the ending location here
    gen.CreateBr(condBlock, body.encoded_location());

    // end block
    gen.SetInsertPoint(endBlock);

}

void ForInLoop::code_gen(Codegen &gen) {
    const auto location = encoded_location();
    const auto exprType = expr->getType()->canonical();
    const auto itrElemType = getIterationElementActualType();

    FunctionDeclaration* iter_data_fn = nullptr;
    FunctionDeclaration* iter_size_fn = nullptr;
    FunctionDeclaration* chunk_begin_fn = nullptr;
    FunctionDeclaration* chunk_valid_fn = nullptr;
    FunctionDeclaration* chunk_current_fn = nullptr;
    FunctionDeclaration* chunk_next_fn = nullptr;
    FunctionDeclaration* chunk_rbegin_fn = nullptr;
    FunctionDeclaration* chunk_previous_fn = nullptr;
    FunctionDeclaration* chunk_total_size_fn = nullptr;
    FunctionDeclaration* iterable_begin_fn = nullptr;
    FunctionDeclaration* iterable_valid_fn = nullptr;
    FunctionDeclaration* iterable_current_fn = nullptr;
    FunctionDeclaration* iterable_next_fn = nullptr;
    if (exprType->kind() != BaseTypeKind::Array) {
        const auto linked = exprType->get_linked_node(true, false);
        if (linked) {
            const auto container = linked->get_members_container();
            if (container) {
                iter_data_fn = gen.implsIndex.get_linear_data_impl(gen.coreNodes, container);
                iter_size_fn = gen.implsIndex.get_linear_size_impl(gen.coreNodes, container);
                chunk_begin_fn = gen.implsIndex.get_chunked_begin_chunks_impl(gen.coreNodes, container);
                chunk_valid_fn = gen.implsIndex.get_chunked_valid_chunk_impl(gen.coreNodes, container);
                chunk_current_fn = gen.implsIndex.get_chunked_current_chunk_impl(gen.coreNodes, container);
                chunk_next_fn = gen.implsIndex.get_chunked_next_chunk_impl(gen.coreNodes, container);
                chunk_rbegin_fn = gen.implsIndex.get_chunked_rbegin_chunks_impl(gen.coreNodes, container);
                chunk_previous_fn = gen.implsIndex.get_chunked_previous_chunk_impl(gen.coreNodes, container);
                chunk_total_size_fn = gen.implsIndex.get_chunked_total_size_impl(gen.coreNodes, container);
                iterable_begin_fn = gen.implsIndex.get_iterable_begin_impl(gen.coreNodes, container);
                iterable_valid_fn = gen.implsIndex.get_iterable_valid_impl(gen.coreNodes, container);
                iterable_current_fn = gen.implsIndex.get_iterable_current_impl(gen.coreNodes, container);
                iterable_next_fn = gen.implsIndex.get_iterable_next_impl(gen.coreNodes, container);
            }
        }
    }

    // 1. Evaluate expression into a variable if needed
    llvm::Value* exprPtr = expr->llvm_pointer(gen);

    if (iteration_kind == ForInLoopIterationKind::Chunked) {
        const auto beginFunc = is_reversed() ? chunk_rbegin_fn : chunk_begin_fn;
        const auto stepFunc = is_reversed() ? chunk_previous_fn : chunk_next_fn;
        const auto cursorIsStruct = beginFunc->returnType->isStructLikeType();
        const auto cursorTy = beginFunc->returnType->llvm_type(gen);
        const auto cursorAlloca = gen.llvm.CreateAlloca(cursorTy, encoded_location());
        if (cursorIsStruct) {
            call_for_in_interface_fn(gen, beginFunc, { exprPtr }, encoded_location(), cursorAlloca);
        } else {
            auto cursorInit = call_for_in_interface_fn(gen, beginFunc, { exprPtr }, encoded_location());
            gen.llvm.CreateStore(cursorInit, cursorAlloca, encoded_location());
        }

        const auto indexType = index_init ? index_init->type->llvm_type(gen) : nullptr;
        if (index_init) {
            const auto indexPtr = gen.llvm.CreateAlloca(indexType, encoded_location());
            index_init->llvm_ptr = indexPtr;
            llvm::Value* initialIdx = nullptr;
            if (is_reversed_counter()) {
                const auto totalCall = gen.builder->CreateCall(chunk_total_size_fn->llvm_func(gen), { exprPtr });
                gen.di.instr(totalCall, encoded_location());
                initialIdx = gen.implicit_cast(totalCall, index_init->type, indexType);
            } else {
                initialIdx = llvm::ConstantInt::get(indexType, 0);
            }
            gen.llvm.CreateStore(initialIdx, index_init->llvm_ptr, encoded_location());
        }

        auto outerCondBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.chunk.cond", gen.current_function);
        auto outerBodyBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.chunk.body", gen.current_function);
        auto innerCondBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.chunk.inner.cond", gen.current_function);
        auto innerBodyBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.chunk.inner.body", gen.current_function);
        auto innerIncBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.chunk.inner.inc", gen.current_function);
        auto outerIncBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.chunk.inc", gen.current_function);
        auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.chunk.end", gen.current_function);

        gen.CreateBr(outerCondBlock, location);

        gen.SetInsertPoint(outerCondBlock);
        auto cursorVal = cursorIsStruct ? cursorAlloca : gen.builder->CreateLoad(cursorTy, cursorAlloca);
        auto validCall = call_for_in_interface_fn(gen, chunk_valid_fn, { exprPtr, cursorVal }, encoded_location());
        gen.CreateCondBr(validCall, outerBodyBlock, endBlock, location);

        gen.SetInsertPoint(outerBodyBlock);
        cursorVal = cursorIsStruct ? cursorAlloca : gen.builder->CreateLoad(cursorTy, cursorAlloca);
        const auto chunkTy = chunk_current_fn->returnType->llvm_type(gen);
        auto chunkAlloca = call_for_in_interface_fn(gen, chunk_current_fn, { exprPtr, cursorVal }, encoded_location());
        auto chunkDataPtr = gen.builder->CreateStructGEP(chunkTy, chunkAlloca, 0);
        auto chunkLenPtr = gen.builder->CreateStructGEP(chunkTy, chunkAlloca, 1);
        auto dataPtr = gen.builder->CreateLoad(gen.builder->getPtrTy(), chunkDataPtr);
        auto lenVal = gen.builder->CreateLoad(chunk_total_size_fn->returnType->llvm_type(gen), chunkLenPtr);

        const auto llvmPtrTy = gen.builder->getPtrTy();
        llvm::Value* loopPtrAlloca = gen.llvm.CreateAlloca(llvmPtrTy, encoded_location());
        llvm::Value* startPtr = dataPtr;
        llvm::Value* endPtr = gen.builder->CreateGEP(itrElemType->llvm_type(gen), dataPtr, { lenVal }, "", gen.inbounds);
        gen.llvm.CreateStore(is_reversed() ? endPtr : startPtr, loopPtrAlloca, encoded_location());
        gen.CreateBr(innerCondBlock, location);

        gen.SetInsertPoint(innerCondBlock);
        auto currPtr = gen.builder->CreateLoad(llvmPtrTy, loopPtrAlloca);
        auto stopPtr = is_reversed() ? startPtr : endPtr;
        auto cmp = gen.builder->CreateICmpNE(currPtr, stopPtr);
        gen.CreateCondBr(cmp, innerBodyBlock, outerIncBlock, location);

        gen.SetInsertPoint(innerBodyBlock);
        if (is_reversed()) {
            llvm::Value* p = gen.builder->CreateLoad(llvmPtrTy, loopPtrAlloca);
            p = gen.builder->CreateGEP(itrElemType->llvm_type(gen), p, { gen.builder->getInt32(-1) }, "", gen.inbounds);
            gen.llvm.CreateStore(p, loopPtrAlloca, encoded_location());
        }
        if (index_init && is_reversed_counter()) {
            llvm::Value* idx = gen.builder->CreateLoad(indexType, index_init->llvm_ptr);
            idx = gen.builder->CreateSub(idx, llvm::ConstantInt::get(idx->getType(), 1));
            gen.llvm.CreateStore(idx, index_init->llvm_ptr, encoded_location());
        }
        id_ptr = gen.builder->CreateLoad(llvmPtrTy, loopPtrAlloca);
        gen.loop_body_gen(body, innerIncBlock, endBlock);
        gen.CreateBr(innerIncBlock, location);

        gen.SetInsertPoint(innerIncBlock);
        if (!is_reversed()) {
            llvm::Value* p = gen.builder->CreateLoad(llvmPtrTy, loopPtrAlloca);
            const auto incPtr = gen.builder->CreateGEP(itrElemType->llvm_type(gen), p, { gen.builder->getInt32(1) }, "", gen.inbounds);
            gen.llvm.CreateStore(incPtr, loopPtrAlloca, encoded_location());
        }
        if (index_init && !is_reversed_counter()) {
            llvm::Value* idx = gen.builder->CreateLoad(indexType, index_init->llvm_ptr);
            idx = gen.builder->CreateAdd(idx, llvm::ConstantInt::get(idx->getType(), 1));
            gen.llvm.CreateStore(idx, index_init->llvm_ptr, encoded_location());
        }
        gen.CreateBr(innerCondBlock, location);

        gen.SetInsertPoint(outerIncBlock);
        cursorVal = cursorIsStruct ? cursorAlloca : gen.builder->CreateLoad(cursorTy, cursorAlloca);
        if (cursorIsStruct) {
            auto nextCursorPtr = call_for_in_interface_fn(gen, stepFunc, { exprPtr, cursorVal }, encoded_location());
            auto nextCursor = gen.builder->CreateLoad(cursorTy, nextCursorPtr);
            gen.llvm.CreateStore(nextCursor, cursorAlloca, encoded_location());
        } else {
            auto nextCursor = call_for_in_interface_fn(gen, stepFunc, { exprPtr, cursorVal }, encoded_location());
            gen.llvm.CreateStore(nextCursor, cursorAlloca, encoded_location());
        }
        gen.CreateBr(outerCondBlock, location);

        gen.SetInsertPoint(endBlock);
        return;
    }

    if (iteration_kind == ForInLoopIterationKind::Iterable) {
        const auto cursorIsStruct = iterable_begin_fn->returnType->isStructLikeType();
        const auto cursorTy = iterable_begin_fn->returnType->llvm_type(gen);
        const auto cursorAlloca = gen.llvm.CreateAlloca(cursorTy, encoded_location());
        if (cursorIsStruct) {
            call_for_in_interface_fn(gen, iterable_begin_fn, { exprPtr }, encoded_location(), cursorAlloca);
        } else {
            auto cursorInit = call_for_in_interface_fn(gen, iterable_begin_fn, { exprPtr }, encoded_location());
            gen.llvm.CreateStore(cursorInit, cursorAlloca, encoded_location());
        }

        const auto indexType = index_init ? index_init->type->llvm_type(gen) : nullptr;
        if (index_init) {
            const auto indexPtr = gen.llvm.CreateAlloca(indexType, encoded_location());
            index_init->llvm_ptr = indexPtr;
            gen.llvm.CreateStore(llvm::ConstantInt::get(indexType, 0), index_init->llvm_ptr, encoded_location());
        }

        auto condBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.iter.cond", gen.current_function);
        auto bodyBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.iter.body", gen.current_function);
        auto incBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.iter.inc", gen.current_function);
        auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.iter.end", gen.current_function);

        gen.CreateBr(condBlock, location);

        gen.SetInsertPoint(condBlock);
        auto cursorVal = cursorIsStruct ? cursorAlloca : gen.builder->CreateLoad(cursorTy, cursorAlloca);
        auto validCall = call_for_in_interface_fn(gen, iterable_valid_fn, { exprPtr, cursorVal }, encoded_location());
        gen.CreateCondBr(validCall, bodyBlock, endBlock, location);

        gen.SetInsertPoint(bodyBlock);
        cursorVal = cursorIsStruct ? cursorAlloca : gen.builder->CreateLoad(cursorTy, cursorAlloca);
        auto currentPtr = call_for_in_interface_fn(gen, iterable_current_fn, { exprPtr, cursorVal }, encoded_location());
        id_ptr = currentPtr;
        gen.loop_body_gen(body, incBlock, endBlock);
        gen.CreateBr(incBlock, location);

        gen.SetInsertPoint(incBlock);
        cursorVal = cursorIsStruct ? cursorAlloca : gen.builder->CreateLoad(cursorTy, cursorAlloca);
        if (cursorIsStruct) {
            auto nextCursorPtr = call_for_in_interface_fn(gen, iterable_next_fn, { exprPtr, cursorVal }, encoded_location());
            auto nextCursor = gen.builder->CreateLoad(cursorTy, nextCursorPtr);
            gen.llvm.CreateStore(nextCursor, cursorAlloca, encoded_location());
        } else {
            auto nextCursor = call_for_in_interface_fn(gen, iterable_next_fn, { exprPtr, cursorVal }, encoded_location());
            gen.llvm.CreateStore(nextCursor, cursorAlloca, encoded_location());
        }
        if (index_init) {
            llvm::Value* idx = gen.builder->CreateLoad(indexType, index_init->llvm_ptr);
            idx = gen.builder->CreateAdd(idx, llvm::ConstantInt::get(idx->getType(), 1));
            gen.llvm.CreateStore(idx, index_init->llvm_ptr, encoded_location());
        }
        gen.CreateBr(condBlock, location);

        gen.SetInsertPoint(endBlock);
        return;
    }

    // 2. Get Data Pointer and Size
    llvm::Value* dataPtr = nullptr;
    llvm::Value* sizeVal = nullptr;

    if (exprType->kind() == BaseTypeKind::Array) {
        dataPtr = gen.builder->CreateStructGEP(exprType->llvm_type(gen), exprPtr, 0);
        const auto arrType = exprType->as_array_type_unsafe();
        sizeVal = gen.builder->getInt64(arrType->get_array_size());
    } else {
        if (iter_data_fn) {
            const auto dataCallInstr = gen.builder->CreateCall(iter_data_fn->llvm_func(gen), { exprPtr });
            dataPtr = dataCallInstr;
            gen.di.instr(dataCallInstr, encoded_location());
        }
        if (iter_size_fn) {
            const auto sizeCallInstr = gen.builder->CreateCall(iter_size_fn->llvm_func(gen), { exprPtr });
            sizeVal = sizeCallInstr;
            gen.di.instr(sizeCallInstr, encoded_location());
        }
    }

    if (!dataPtr || !sizeVal) return;

    const auto llvmPtrTy = gen.builder->getPtrTy();

    // 3. Initialize loop variables
    llvm::Type* elemPtrTy = dataPtr->getType();
    llvm::Value* loopPtrAlloca = gen.llvm.CreateAlloca(llvmPtrTy, encoded_location());

    llvm::Value* startPtr = dataPtr;
    llvm::Value* endPtr = gen.builder->CreateGEP(itrElemType->llvm_type(gen), dataPtr, { sizeVal }, "", gen.inbounds);

    if (is_reversed()) {
        gen.llvm.CreateStore(endPtr, loopPtrAlloca, encoded_location());
    } else {
        gen.llvm.CreateStore(startPtr, loopPtrAlloca, encoded_location());
    }

    // calculate the llvm type for index
    const auto indexType = index_init ? index_init->type->llvm_type(gen) : nullptr;
    // creating a variable for the index counter
    if (index_init) {
        const auto indexPtr = gen.llvm.CreateAlloca(indexType, encoded_location());
        index_init->llvm_ptr = indexPtr;
        llvm::Value* initialIdx = nullptr;
        if (is_reversed_counter()) {
            initialIdx = sizeVal;
        } else {
            initialIdx = llvm::ConstantInt::get(indexType, 0);
        }
        gen.llvm.CreateStore(initialIdx, index_init->llvm_ptr, encoded_location());
    }

    // 4. Blocks
    auto condBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.cond", gen.current_function);
    auto bodyBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.body", gen.current_function);
    auto incBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.inc", gen.current_function);
    auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "forin.end", gen.current_function);

    gen.CreateBr(condBlock, location);

    // 5. Condition Block
    gen.SetInsertPoint(condBlock);
    llvm::Value* currPtr = gen.builder->CreateLoad(elemPtrTy, loopPtrAlloca);
    llvm::Value* stopPtr = is_reversed() ? startPtr : endPtr;
    llvm::Value* cmp = gen.builder->CreateICmpNE(currPtr, stopPtr);
    gen.CreateCondBr(cmp, bodyBlock, endBlock, location);

    // 6. Body Block
    gen.SetInsertPoint(bodyBlock);

    // decrementing the loop pointer (after loading)
    if (is_reversed()) {
        llvm::Value* p = gen.builder->CreateLoad(elemPtrTy, loopPtrAlloca);
        p = gen.builder->CreateGEP(itrElemType->llvm_type(gen), p, { gen.builder->getInt32(-1) }, "", gen.inbounds);
        gen.llvm.CreateStore(p, loopPtrAlloca, encoded_location());
    }

    // decrementing the counter variable
    if (index_init && is_reversed_counter()) {
        llvm::Value* idx = gen.builder->CreateLoad(indexType, index_init->llvm_ptr);
        idx = gen.builder->CreateSub(idx, llvm::ConstantInt::get(idx->getType(), 1));
        gen.llvm.CreateStore(idx, index_init->llvm_ptr, encoded_location());
    }

    // loading the pointer of loop
    // we do this once, this will be optimized out if never used
    id_ptr = gen.builder->CreateLoad(llvmPtrTy, loopPtrAlloca);

    // generating the actual body (user provided)
    gen.loop_body_gen(body, incBlock, endBlock);
    gen.CreateBr(incBlock, location);

    // 7. Increment Block
    gen.SetInsertPoint(incBlock);

    // incrementing the loop pointer
    if (!is_reversed()) {
        llvm::Value* p = gen.builder->CreateLoad(elemPtrTy, loopPtrAlloca);
        const auto incPtr = gen.builder->CreateGEP(itrElemType->llvm_type(gen), p, { gen.builder->getInt32(1) }, "", gen.inbounds);
        gen.llvm.CreateStore(incPtr, loopPtrAlloca, encoded_location());
    }

    // incrementing the counter index
    if (index_init && !is_reversed_counter()) {
        llvm::Value* idx = gen.builder->CreateLoad(indexType, index_init->llvm_ptr);
        idx = gen.builder->CreateAdd(idx, llvm::ConstantInt::get(idx->getType(), 1));
        gen.llvm.CreateStore(idx, index_init->llvm_ptr, encoded_location());
    }

    gen.CreateBr(condBlock, location);

    // 8. End Block
    gen.SetInsertPoint(endBlock);
}

llvm::Value* ForInLoop::llvm_load(Codegen& gen, SourceLocation location) {
    if (is_reference()) return id_ptr;
    auto idStrRef = llvm::StringRef(id.data(), id.size());
    const auto loadInst = gen.builder->CreateLoad(llvm_type(gen), id_ptr, idStrRef);
    gen.di.instr(loadInst, location);
    return loadInst;
}

bool ForInLoop::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return elem_type->linked_node()->add_child_index(gen, indexes, name);
}

#endif

void ForLoop::stopInterpretation() {
    stoppedInterpretation = true;
}

void ForInLoop::stopInterpretation() {
    attrs.stoppedInterpretation = true;
}
