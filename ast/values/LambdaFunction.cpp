// Copyright (c) Qinetik 2024.

#include "LambdaFunction.h"
#include "ast/structures/FunctionDeclaration.h"
#include "FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/statements/Return.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "ast/types/VoidType.h"
#include "ast/base/LoopASTNode.h"
#include "ast/statements/VarInit.h"
#include "ast/values/StructValue.h"

llvm::Type *LambdaFunction::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *LambdaFunction::llvm_value(Codegen &gen) {
    if(func_type == nullptr) {
        gen.error("Cannot generate lambda function for unknown type");
        return nullptr;
    }
    return gen.create_nested_function("lambda", func_type->llvm_func_type(gen), scope);
}

std::unique_ptr<BaseType> LambdaFunction::create_type() const {
    return std::unique_ptr<BaseType>(func_type->copy());
}

#endif

LambdaFunction::LambdaFunction(
        std::vector<std::string> captureList,
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

void link_body(LambdaFunction* fn, SymbolResolver &linker) {
    linker.scope_start();
    for (auto &param: fn->params) {
        param->declare_and_link(linker);
    }
    fn->scope.declare_and_link(linker);
    linker.scope_end();
}

void LambdaFunction::link(SymbolResolver &linker) {

    linker.info("lambda function type not found, deducing function type by visiting lambda body (expensive operation) performed");

    // finding return type
    auto returnType = find_return_type(scope.nodes);

    // linking params and their types before copying their types
    linker.scope_start();
    for (auto &param: params) {
        param->declare_and_link(linker);
    }

    // copying function param
    func_params funcParams;
    for(auto& param : params) {
        funcParams.emplace_back(param->copy());
    }

    func_type = std::make_unique<FunctionType>(std::move(funcParams), std::unique_ptr<BaseType>(returnType), isVariadic, !captureList.empty());

    scope.declare_and_link(linker);
    linker.scope_end();

}

void LambdaFunction::link(SymbolResolver &linker, VarInitStatement *stmnt) {
    if(stmnt->type.has_value()) {
        func_type = std::shared_ptr<FunctionType>((FunctionType*) stmnt->type.value()->copy());
        link_body(this, linker);
    } else {
        link(linker);
    }
}

void LambdaFunction::link(SymbolResolver &linker, StructValue *value, const std::string &name) {
    func_type = std::shared_ptr<FunctionType>((FunctionType*) value->definition->child(name)->create_value_type().release());
    link_body(this, linker);
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

    link_body(this, linker);

}

void LambdaFunction::link(SymbolResolver &linker, ReturnStatement *returnStmt) {

    auto retType = returnStmt->declaration->returnType.get();
    if(retType->function_type() == nullptr) {
        linker.error("cannot return lambda, return type of a function is not a function");
        return;
    }

    func_type = std::shared_ptr<FunctionType>(retType->function_type(), [](BaseType*){});

    link_body(this, linker);

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