// Copyright (c) Qinetik 2024.

#include "LambdaFunction.h"
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

llvm::AllocaInst *LambdaFunction::llvm_allocate(Codegen &gen, const std::string &identifier) {
    if(func_type->isCapturing) {
        auto lamb = llvm_value(gen);
        return llvm_allocate_with(gen, identifier, gen.pack_lambda((llvm::Function*) lamb, captured_struct), llvm_type(gen));
    } else {
        return Value::llvm_allocate(gen, identifier);
    }
}

llvm::Value *LambdaFunction::llvm_value(Codegen &gen) {
    if(func_type == nullptr) {
        gen.error("Cannot generate lambda function for unknown type");
        return nullptr;
    }

    auto nested = gen.create_nested_function("lambda", func_type->llvm_func_type(gen), scope);
    if(!captureList.empty()) {
        // storing captured variables in a struct
        captured_struct = capture_struct(gen);
    }
//    else {
//        captured_struct = llvm::ConstantPointerNull::get(llvm::PointerType::get(*gen.ctx, 0));
//    }
    return nested;
}

llvm::Value *LambdaFunction::llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) {
    if(!func_type->isCapturing) {
        return llvm_value(gen);
    } else {
        auto value = (llvm::Function *) llvm_value(gen); // called first so that captured_struct is set
        return gen.pack_lambda(value, captured_struct);
    }
}

llvm::FunctionType *LambdaFunction::llvm_func_type(Codegen &gen) {
    return func_type->llvm_func_type(gen);
}

llvm::Type *LambdaFunction::capture_struct_type(Codegen &gen) {
    std::vector<llvm::Type*> members;
    for(auto& cap : captureList) {
        members.emplace_back(cap->llvm_type(gen));
    }
    return llvm::StructType::get(*gen.ctx, members);
}

#endif

std::unique_ptr<BaseType> LambdaFunction::create_type() const {
    auto prev_capturing = func_type->isCapturing;
    func_type->isCapturing = !captureList.empty();
    auto copied = std::unique_ptr<BaseType>(func_type->copy());
    func_type->isCapturing = prev_capturing;
    return copied;
}

LambdaFunction::LambdaFunction(
        std::vector<std::unique_ptr<CapturedVariable>> captureList,
        std::vector<std::unique_ptr<FunctionParam>> params,
        bool isVariadic,
        Scope scope
) : captureList(std::move(captureList)), params(std::move(params)), isVariadic(isVariadic), scope(std::move(scope)) {

}

BaseType* find_return_type(std::vector<std::unique_ptr<ASTNode>>& nodes) {
    for(auto& node : nodes) {
        if(node->as_return() != nullptr) {
            auto returnStmt = node->as_return();
            if(returnStmt->value.has_value()) {
                return returnStmt->value.value()->create_type().release();
            } else {
                return new VoidType();
            }
        } else if(node->as_loop_ast() != nullptr) {
            auto found = find_return_type(node->as_loop_ast()->body.nodes);
            if(found != nullptr) {
                return found;
            }
        }
    }
    return new VoidType();
}

void link_params_and_caps(LambdaFunction* fn, SymbolResolver &linker) {
    for(auto& cap : fn->captureList) {
        cap->declare_and_link(linker);
    }
    for (auto &param : fn->params) {
        param->declare_and_link(linker);
    }
}

void link_full(LambdaFunction* fn, SymbolResolver &linker) {
    linker.scope_start();
    link_params_and_caps(fn, linker);
    fn->scope.declare_and_link(linker);
    linker.scope_end();
}

void LambdaFunction::link(SymbolResolver &linker) {

#ifdef DEBUG
    linker.info("lambda function type not found, deducing function type by visiting lambda body (expensive operation) performed");
#endif

    // linking params and their types before copying their types
    link_full(this, linker);

    // finding return type
    auto returnType = find_return_type(scope.nodes);

    // copying function param
    func_params funcParams;
    for(auto& param : params) {
        funcParams.emplace_back(param->copy());
    }

    func_type = std::make_unique<FunctionType>(std::move(funcParams), std::unique_ptr<BaseType>(returnType), isVariadic, !captureList.empty());

}

void LambdaFunction::link(SymbolResolver &linker, VarInitStatement *stmnt) {
    if(stmnt->type.has_value()) {
        func_type = std::shared_ptr<FunctionType>((FunctionType*) stmnt->create_value_type().release());
        link_full(this, linker);
    } else {
        link(linker);
    }
}

void LambdaFunction::link(SymbolResolver &linker, StructValue *value, const std::string &name) {
    func_type = std::shared_ptr<FunctionType>((FunctionType*) value->definition->child(name)->create_value_type().release());
    link_full(this, linker);
}

void LambdaFunction::link(SymbolResolver &linker, FunctionCall *call, unsigned int index) {

    // if the linked is a function decl, it will be its type
    // if it's a variable with a lambda type, it will be its type
    auto linkedType = call->linked()->create_value_type();

    // this is not a function, this error has been probably caught by function call
    if(linkedType->function_type() == nullptr) {
        return;
    }

    // get the type of parameter for the function
    auto paramType = linkedType->function_type()->params[index]->create_value_type()->copy();

    if(paramType->function_type() == nullptr) {
        linker.error("cannot pass a lambda, because the function expects a different type : " + paramType->representation() + " for parameter at " + std::to_string(index));
        return;
    }

    func_type = std::shared_ptr<FunctionType>(paramType->function_type());

    link_full(this, linker);

}

void LambdaFunction::link(SymbolResolver &linker, ReturnStatement *returnStmt) {

    if(returnStmt->declaration != nullptr) {
        auto retType = returnStmt->declaration->returnType->copy();
        if(retType->function_type() == nullptr) {
            linker.error("cannot return lambda, return type of a function is not a function");
            return;
        }
        func_type = std::shared_ptr<FunctionType>(retType->function_type(), [](BaseType*){});
    } else {
        link(linker);
        return;
    }

    link_full(this, linker);

}

std::string LambdaFunction::representation() const {
    std::string rep("[");
    unsigned i = 0;
    unsigned size = captureList.size();
    while(i < size) {
        rep.append(captureList[i]->representation());
        if(i < size - 1){
            rep.append(1, ',');
        }
        i++;
    }
    rep.append("](");
    i = 0;
    size = params.size();
    while(i < size) {
        rep.append(params[i]->representation());
        if(i < size - 1){
            rep.append(1, ',');
        }
        i++;
    }
    rep.append(") => {\n");
    rep.append(scope.representation());
    rep.append("\n}");
    return rep;
}

ValueType LambdaFunction::value_type() const {
    return ValueType::Lambda;
}