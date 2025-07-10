// Copyright (c) Chemical Language Foundation 2025.

#include "LambdaFunction.h"

#include <memory>
#include "ast/structures/FunctionDeclaration.h"
#include "FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/statements/Return.h"
#include "ast/structures/CapturedVariable.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/VoidType.h"
#include "ast/statements/VarInit.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/base/LoopASTNode.h"
#include "ast/values/StructValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/LinkedType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type *LambdaFunction::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::AllocaInst* LambdaFunction::capture_struct(Codegen &gen) {
    // storing captured variables in a struct
    auto capturedStructType = capture_struct_type(gen);
    auto capturedAlloca = gen.llvm.CreateAlloca(capturedStructType, (Value*) this);
    unsigned i = 0;
    while(i < captureList.size()) {
        auto cap = captureList[i];
        auto ptr = gen.llvm.CreateStructGEP(capturedStructType, capturedAlloca, i);
        if(cap->capture_by_ref) {
            gen.llvm.CreateStore(cap->linked->llvm_pointer(gen), ptr, cap);
        } else {
            const auto type = cap->linked->known_type();
            if(type->isStructLikeType()) {
                // we set the drop flag for linked node to false so destructor is not called on it
                gen.set_drop_flag_for_node(cap->linked, false, cap->encoded_location());
                // we memcpy struct type into the captured struct
                gen.memcpy_struct(type->llvm_type(gen), ptr, cap->linked->llvm_pointer(gen), cap->encoded_location());
            } else {
                const auto instr = gen.llvm.CreateStore(cap->linked->llvm_load(gen, cap->encoded_location()), ptr, cap);
            }
        }
        i++;
    }
    return (llvm::AllocaInst*) capturedAlloca;
}

llvm::Value* packed_lambda_val(Codegen& gen, LambdaFunction* lambda) {
    if(lambda->isCapturing()) {
        auto captured = lambda->captured_struct;
        if(!captured) {
            captured = llvm::ConstantPointerNull::get(llvm::PointerType::get(*gen.ctx, 0));
        }
        return gen.pack_fat_pointer(lambda->func_ptr, captured, lambda->Value::encoded_location());
    } else {
        return lambda->func_ptr;
    }
}

void LambdaFunction::generate_captured_destructor(Codegen &gen) {
    if(!has_destructor_for_capture()) {
        // capture struct doesn't have a destructor
        return;
    }
    const auto destrFuncType = llvm::FunctionType::get(gen.builder->getVoidTy(), {gen.builder->getPtrTy()}, false);

    // creating temporary ast nodes for captured destructor
    VoidType voidType;
    PointerType ptrType(&voidType, true);
    FunctionParam functionParam("self", TypeLoc(&ptrType, ZERO_LOC), 0, nullptr, false, nullptr, ZERO_LOC);
    FunctionDeclaration funcDecl(
            LocatedIdentifier("lambda_cap_destr"),
            TypeLoc(&voidType, ZERO_LOC),
            false,
            nullptr,
            ZERO_LOC,
            AccessSpecifier::Internal,
            false
    );
    funcDecl.params.emplace_back(&functionParam);
    funcDecl.body.emplace(Scope(&funcDecl, ZERO_LOC));

    const auto prev_destroy_scope = gen.destroy_current_scope;
    const auto prev_block_ended = gen.has_current_block_ended;
    const auto prev_block = gen.builder->GetInsertBlock();
    const auto prev_current_func = gen.current_function;
    const auto prev_func_type = gen.current_func_type;

    gen.SetInsertPoint(nullptr);
    const auto nested_function = gen.create_function("lambda_cap_destr", destrFuncType, AccessSpecifier::Internal);
    gen.current_function = nested_function;
    // this begins the function scope by creating a di subprogram
    gen.di.start_nested_function_scope(&funcDecl, nested_function);

    capturedDestructor = nested_function;
    auto captured = nested_function->getArg(0);
    for(const auto cap : captureList) {
        llvm::Function* destr;
        const auto destrDecl = gen.determine_destructor_for(cap->known_type(), destr);
        if(destrDecl) {
            const auto capturedMember = gen.builder->CreateStructGEP(capture_struct_type(gen), captured, cap->index);
            const auto callInst = gen.builder->CreateCall(destr, { capturedMember });
            gen.di.instr(callInst, encoded_location());
        }
    }
    gen.CreateRet(nullptr, encoded_location());

    // this will end the function scope we started by creating a di subprogram above
    gen.di.end_function_scope();
    gen.has_current_block_ended = prev_block_ended;
    gen.SetInsertPoint(prev_block);
    gen.current_function = prev_current_func;
    gen.destroy_current_scope = prev_destroy_scope;

}

llvm::Value* LambdaFunction::llvm_value_unpacked(Codegen &gen, BaseType* expected_type) {
    if(func_ptr) {
        return func_ptr;
    }
    func_ptr = gen.create_nested_function("lambda", FunctionType::llvm_func_type(gen), this, scope);
    if(!captureList.empty()) {
        // storing captured variables in a struct
        captured_struct = capture_struct(gen);
        generate_captured_destructor(gen);
    }
    return func_ptr;
}

llvm::Value *LambdaFunction::llvm_value(Codegen &gen, BaseType* expected_type) {
    if(func_ptr) {
        return packed_lambda_val(gen, this);
    }
    func_ptr = gen.create_nested_function("lambda", FunctionType::llvm_func_type(gen), this, scope);
    if(!captureList.empty()) {
        // storing captured variables in a struct
        captured_struct = capture_struct(gen);
        generate_captured_destructor(gen);
    }
    return packed_lambda_val(gen, this);
}

void LambdaFunction::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    // this refers to the lambda pointer
    const auto unpacked = llvm_value_unpacked(gen, nullptr);
    if(captureList.empty()) {
        gen.assign_store(lhs, lhsPtr, this, unpacked, encoded_location());
    } else {
        const auto lhsType = lhs->known_type();
        const auto can = lhsType->canonical();
        if(can->kind() == BaseTypeKind::CapturingFunction) {
            gen.mutate_capturing_function(can, this, lhsPtr);
        } else {
            gen.error("capturing lambda being assigned to non capturing type", this);
        }
    }
}

llvm::Type *LambdaFunction::capture_struct_type(Codegen &gen) {
    std::vector<llvm::Type*> members;
    for(auto& cap : captureList) {
        members.emplace_back(cap->llvm_type(gen));
    }
    return llvm::StructType::get(*gen.ctx, members);
}

#endif

bool LambdaFunction::has_destructor_for_capture() {
    if(captureList.empty()) {
        // since user did not capture any variable, we don't need to generate a destructor
        return false;
    }
    for(const auto cap : captureList) {
        const auto ty = cap->known_type();
        if(ty && ty->get_destructor() != nullptr){
            return true;
        }
    }
    return false;
}

BaseType* LambdaFunction::create_type(ASTAllocator& allocator) {
    auto func_type = FunctionType::copy(allocator);
    func_type->setIsCapturing(!captureList.empty());
    return func_type;
}