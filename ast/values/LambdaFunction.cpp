// Copyright (c) Qinetik 2024.

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
    unsigned i = 0;
    while(i < captureList.size()) {
        auto cap = captureList[i].get();
        auto ptr = gen.builder->CreateStructGEP(capturedStructType, capturedAlloca, i);
        if(cap->capture_by_ref) {
            gen.builder->CreateStore(cap->linked->llvm_pointer(gen), ptr);
        } else {
            gen.builder->CreateStore(cap->linked->llvm_load(gen), ptr);
        }
        i++;
    }
    return capturedAlloca;
}

llvm::Value* packed_lambda_val(Codegen& gen, LambdaFunction* lambda) {
    if(lambda->isCapturing) {
        auto captured = lambda->captured_struct;
        if(!captured) {
            captured = llvm::ConstantPointerNull::get(llvm::PointerType::get(*gen.ctx, 0));
        }
        return gen.pack_fat_pointer(lambda->func_ptr, captured);
    } else {
        return lambda->func_ptr;
    }
}

llvm::Value *LambdaFunction::llvm_value(Codegen &gen, BaseType* expected_type) {
    if(func_ptr) {
        gen.error("llvm_value called multiple times on LambdaFunction", (Value*) this);
    }
    func_ptr = gen.create_nested_function("lambda", FunctionType::llvm_func_type(gen), this, scope);
    if(!captureList.empty()) {
        // storing captured variables in a struct
        captured_struct = capture_struct(gen);
    }
    return packed_lambda_val(gen, this);
}

llvm::Type *LambdaFunction::capture_struct_type(Codegen &gen) {
    std::vector<llvm::Type*> members;
    for(auto& cap : captureList) {
        members.emplace_back(cap->llvm_type(gen));
    }
    return llvm::StructType::get(*gen.ctx, members);
}

#endif

std::unique_ptr<BaseType> LambdaFunction::create_type() {
    auto func_type = std::unique_ptr<FunctionType>((FunctionType*) FunctionType::copy());
    func_type->isCapturing = !captureList.empty();
    return func_type;
}

hybrid_ptr<BaseType> LambdaFunction::get_base_type() {
    auto func_type = (FunctionType*) FunctionType::copy();
    func_type->isCapturing = !captureList.empty();
    return hybrid_ptr<BaseType> { func_type, true };
}

LambdaFunction::LambdaFunction(
        std::vector<CapturedVariable*> captureList,
        std::vector<FunctionParam*> params,
        bool isVariadic,
        Scope scope,
        CSTToken* token
) : captureList(std::move(captureList)), FunctionType(std::move(params), nullptr, isVariadic, !captureList.empty(), token), scope(std::move(scope)) {

}

BaseType* find_return_type(std::vector<std::unique_ptr<ASTNode>>& nodes) {
    for(auto& node : nodes) {
        if(node->as_return() != nullptr) {
            auto returnStmt = node->as_return();
            if(returnStmt->value) {
                return returnStmt->value->create_type().release();
            } else {
                return new VoidType(nullptr);
            }
        } else {
            const auto loop_ast = node->as_loop_ast();
            if(loop_ast) {
                auto found = find_return_type(node->as_loop_ast()->body.nodes);
                if (found != nullptr) {
                    return found;
                }
            }
        }
    }
    return new VoidType(nullptr);
}

void link_params_and_caps(LambdaFunction* fn, SymbolResolver &linker) {
    for(auto& cap : fn->captureList) {
        cap->declare_and_link(linker, (std::unique_ptr<ASTNode>&) cap);
    }
    for (auto &param : fn->params) {
        param->declare_and_link(linker, (std::unique_ptr<ASTNode>&) param);
    }
}

void link_full(LambdaFunction* fn, SymbolResolver &linker) {
    linker.scope_start();
    link_params_and_caps(fn, linker);
    fn->scope.link_sequentially(linker);
    linker.scope_end();
}

bool LambdaFunction::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr, BaseType *expected_type) {

    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;

    auto func_type = expected_type ? expected_type->function_type() : nullptr;

    if(!func_type) {

#ifdef DEBUG
        linker.info("deducing lambda function type by visiting body", (Value*) this);
#endif

        // linking params and their types before copying their types
        link_full(this, linker);

        // finding return type
        auto found_return_type = find_return_type(scope.nodes);
        returnType = std::unique_ptr<BaseType>(found_return_type);

    } else {

        if(!params.empty()) {
            auto& param = params[0];
            if((param->name == "self" || param->name == "this") && (!param->type || param->type->kind() == BaseTypeKind::Void)) {
                param->type = func_type->params[0]->type->copy_unique();
            }
        }

        link(linker, func_type);
        link_full(this, linker);

    }

    linker.current_func_type = prev_func_type;

    return true;

}

void link_given_params(SymbolResolver& resolver, const std::vector<std::unique_ptr<FunctionParam>>& params) {
    for(auto& param : params) {
        if(param->type) {
            param->type->link(resolver, param->type);
        }
    }
}

void copy_func_params_types(const std::vector<std::unique_ptr<FunctionParam>>& from_params, std::vector<std::unique_ptr<FunctionParam>>& to_params, SymbolResolver& resolver, Value* debug_value) {
    if(to_params.size() > from_params.size()) {
        resolver.error("Lambda function type expects " + std::to_string(from_params.size()) + " however given " + std::to_string(to_params.size()), debug_value);
        return;
    }
    auto total = from_params.size();
    auto start = 0;
    while(start < total) {
        const auto from_param = from_params[start].get();
        if(start >= to_params.size()) {
            to_params.emplace_back(nullptr);
        }
        const auto to_param = to_params[start].get();
        if(!to_param->type) {
            to_param->type = std::unique_ptr<BaseType>(from_param->type->copy());
        } else if(!to_param->type->is_same(from_param->type.get())) {
            resolver.error("Lambda function param at index " + std::to_string(start) + " with type " + from_param->type->representation() + ", redeclared with type " + to_param->type->representation(), debug_value);
        }
        start++;
    }
}

bool LambdaFunction::link(SymbolResolver &linker, FunctionType* func_type) {
    link_given_params(linker, params);
    copy_func_params_types(func_type->params, params, linker, this);
    if(!returnType) {
        returnType = std::unique_ptr<BaseType>(func_type->returnType->copy());
    } else if(!returnType->is_same(func_type->returnType.get())) {
        linker.error("Lambda function type expected return type to be " + func_type->returnType->representation() + " but got lambda with return type " + returnType->representation(), (Value*) this);
    }
    isCapturing = func_type->isCapturing;
    return true;
}

ValueType LambdaFunction::value_type() const {
    return ValueType::Lambda;
}