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
    auto capturedAlloca = gen.builder->CreateAlloca(capturedStructType);
    gen.di.instr(capturedAlloca, (Value*) this);
    unsigned i = 0;
    while(i < captureList.size()) {
        auto cap = captureList[i];
        auto ptr = gen.builder->CreateStructGEP(capturedStructType, capturedAlloca, i);
        if(cap->capture_by_ref) {
            const auto instr = gen.builder->CreateStore(cap->linked->llvm_pointer(gen), ptr);
            gen.di.instr(instr, cap);
        } else {
            const auto instr = gen.builder->CreateStore(cap->linked->llvm_load(gen, cap->encoded_location()), ptr);
            gen.di.instr(instr, cap);
        }
        i++;
    }
    return capturedAlloca;
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

BaseType* find_return_type(ASTAllocator& allocator, std::vector<ASTNode*>& nodes) {
    for(const auto node : nodes) {
        if(node->as_return() != nullptr) {
            auto returnStmt = node->as_return();
            if(returnStmt->value) {
                const auto created = returnStmt->value->create_type(allocator);
                if(created) {
                    return created;
                }
            } else {
                return new (allocator.allocate<VoidType>()) VoidType();
            }
        } else {
            const auto loop_ast = node->as_loop_ast();
            if(loop_ast) {
                auto found = find_return_type(allocator, node->as_loop_ast()->body.nodes);
                if (found != nullptr) {
                    return found;
                }
            }
        }
    }
    return new (allocator.allocate<VoidType>()) VoidType();
}

bool link_params_and_caps(LambdaFunction* fn, SymbolResolver &linker, bool link_param_types) {
    bool result = true;
    for(const auto cap : fn->captureList) {
        if(!cap->declare_and_link(linker)) {
            result = false;
        }
    }
    for (auto& param : fn->params) {
        if(link_param_types) {
            if(!param->link_param_type(linker)) {
                result = false;
            }
        }
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    return result;
}

bool link_full(LambdaFunction* fn, SymbolResolver &linker, bool link_param_types) {
    linker.scope_start();
    const auto result = link_params_and_caps(fn, linker, link_param_types);
    fn->scope.link_sequentially(linker);
    linker.scope_end();
    return result;
}

bool LambdaFunction::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {

    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;

    auto func_type = expected_type ? expected_type->canonical()->get_function_type() : nullptr;

    if(!func_type) {

#ifdef DEBUG
        linker.info("deducing lambda function type by visiting body", (Value*) this);
#endif

        // linking params and their types
        auto result = link_full(this, linker, true);

        // finding return type
        auto found_return_type = find_return_type(*linker.ast_allocator, scope.nodes);

        returnType = {found_return_type, get_location()};

        if(result) {
            data.signature_resolved = true;
        }

    } else {

        if(!params.empty()) {
            auto& param = params[0];
            if((param->name == "self" || param->name == "this") && (!param->type || param->type->kind() == BaseTypeKind::Void)) {
                param->type = func_type->params[0]->type;
            }
        }

        link(linker, func_type);
        if(link_full(this, linker, false)) {
            data.signature_resolved = true;
        }

    }

    if(!captureList.empty()) {

        if(prev_func_type) {
            for (const auto captured: captureList) {
                if (captured->capture_by_ref) {
                    continue;
                }
                // we have to allocate an identifier to mark it moved
                // maybe design for this should change a little
                const auto identifier = new(linker.ast_allocator->allocate<VariableIdentifier>()) VariableIdentifier(
                        captured->name, captured->encoded_location(), false
                );
                identifier->linked = captured->linked;
                // we must move the identifiers in capture list
                prev_func_type->mark_moved_value(linker.allocator, identifier, captured->linked->known_type(), linker, false);
            }
        }

        setIsCapturing(true);

    }

    linker.current_func_type = prev_func_type;

    return true;

}

void copy_func_params_types(const std::vector<FunctionParam*>& from_params, std::vector<FunctionParam*>& to_params, SymbolResolver& resolver, Value* debug_value) {
    if(to_params.size() > from_params.size()) {
        resolver.error(debug_value) << "Lambda function type expects " << std::to_string(from_params.size()) << " however given " << std::to_string(to_params.size());
        return;
    }
    auto total = from_params.size();
    auto start = 0;
    while(start < total) {
        const auto from_param = from_params[start];
        if(start >= to_params.size()) {
            to_params.emplace_back(nullptr);
        }
        const auto to_param = to_params[start];
        if(!to_param || !to_param->type || to_param->is_implicit() || from_param->is_implicit()) {
            const auto copied = from_param->copy(*resolver.ast_allocator);
            // change the name to what user wants
            if(to_param) {
                copied->name = to_param->name;
            }
            to_params[start] = copied;
        } else {
            // link the given parameter
            to_param->link_param_type(resolver);
            // check it's type is same as the from parameter
            if(!to_param->type->is_same(from_param->type)) {
                resolver.error(debug_value) << "Lambda function param at index " << std::to_string(start) << " with type " << from_param->type->representation() << ", redeclared with type " << to_param->type->representation();
            }
        }
        start++;
    }
}

bool LambdaFunction::link(SymbolResolver &linker, FunctionType* func_type) {
    copy_func_params_types(func_type->params, params, linker, this);
    if(!returnType) {
        returnType = func_type->returnType.copy(*linker.ast_allocator);
    } else if(!returnType->is_same(func_type->returnType)) {
        linker.error((Value*) this) << "Lambda function type expected return type to be " << func_type->returnType->representation() << " but got lambda with return type " << returnType->representation();
    }
    setIsCapturing(func_type->isCapturing());
    return true;
}