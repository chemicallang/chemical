// Copyright (c) Qinetik 2024.

#include "LambdaFunction.h"
#include "ast/structures/FunctionDeclaration.h"
#include "FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/statements/Return.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *LambdaFunction::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *LambdaFunction::llvm_value(Codegen &gen) {
    if(func_type == nullptr) {
        gen.error("Cannot generate lambda function for unknown type");
        return nullptr;
    }
    return gen.create_nested_function("lambda", (llvm::FunctionType*) func_type->llvm_type(gen), scope);
}

#endif

LambdaFunction::LambdaFunction(
        std::vector<std::string> captureList,
        std::vector<std::unique_ptr<FunctionParam>> params,
        bool isVariadic,
        Scope scope
) : captureList(std::move(captureList)), params(std::move(params)), isVariadic(isVariadic), scope(std::move(scope)) {

}

void LambdaFunction::link(SymbolResolver &linker, FunctionCall *call, unsigned int index) {

    // if the linked is a function decl, it will be its type
    // if it's a variable with a lambda type, it will be its type
    auto linkedType = call->linked->create_value_type();

    // this is not a function, this error has been probably caught by function call
    if(linkedType->function_type() == nullptr) {
        return;
    }

    // get the type of parameter for the function
    auto paramType = linkedType->function_type()->params[index]->create_value_type()->copy();

    if(paramType->function_type() == nullptr) {
        linker.error("cannot pass a lambda, because the function " + call->name + " expects a different type : " + paramType->representation() + " for parameter at " + std::to_string(index));
        return;
    }

    func_type = std::shared_ptr<FunctionType>(paramType->function_type());

    for (auto &param: params) {
        linker.declare(param->name, param.get());
    }
    scope.declare_and_link(linker);
    for (auto &param: params) {
        linker.erase(param->name);
    }

}

void LambdaFunction::link(SymbolResolver &linker, ReturnStatement *returnStmt) {

    auto retType = returnStmt->declaration->returnType.get();
    if(retType->function_type() == nullptr) {
        linker.error("cannot return lambda, return type of a function is not a function");
        return;
    }

    func_type = std::shared_ptr<FunctionType>(retType->function_type(), [](BaseType*){});

    for (auto &param: params) {
        linker.declare(param->name, param.get());
    }
    scope.declare_and_link(linker);
    for (auto &param: params) {
        linker.current.erase(param->name);
    }

}

std::string LambdaFunction::representation() const {
    std::string rep("[");
    unsigned i = 0;
    unsigned size = captureList.size();
    while(i < size) {
        rep.append(captureList[i]);
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