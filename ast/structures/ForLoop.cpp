// Copyright (c) Chemical Language Foundation 2025.

#include "ForLoop.h"
#include "ForInLoop.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/MembersContainer.h"
#include "compiler/symres/ImplementationsIndex.h"

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

    FunctionDeclaration* iter_data_fn = nullptr;
    FunctionDeclaration* iter_size_fn = nullptr;
    if (exprType->kind() != BaseTypeKind::Array) {
        const auto linked = exprType->get_linked_node(true, false);
        if (linked) {
            const auto container = linked->get_members_container();
            if (container) {
                iter_data_fn = gen.implsIndex.get_linear_data_impl(gen.coreNodes, container);
                iter_size_fn = gen.implsIndex.get_linear_size_impl(gen.coreNodes, container);
            }
        }
    }

    // 1. Evaluate expression into a variable if needed
    llvm::Value* exprPtr = expr->llvm_pointer(gen);

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
    llvm::Value* endPtr = gen.builder->CreateGEP(elem_type->llvm_type(gen), dataPtr, { sizeVal }, "", gen.inbounds);

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
        p = gen.builder->CreateGEP(elem_type->llvm_type(gen), p, { gen.builder->getInt32(-1) }, "", gen.inbounds);
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
        const auto incPtr = gen.builder->CreateGEP(elem_type->llvm_type(gen), p, { gen.builder->getInt32(1) }, "", gen.inbounds);
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

#endif

void ForLoop::stopInterpretation() {
    stoppedInterpretation = true;
}

void ForInLoop::stopInterpretation() {
    attrs.stoppedInterpretation = true;
}