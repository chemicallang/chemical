// Copyright (c) Qinetik 2024.

#include "FunctionCall.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *FunctionCall::llvm_value(Codegen &gen) {
    auto fn = gen.module->getFunction(name);
    if (fn == nullptr) {
        gen.error("function with name " + name + " does not exist");
        return nullptr;
    }
    std::vector<llvm::Value *> args(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        args[i] = values[i]->llvm_value(gen);
        // Ensure proper type promotion for float values passed to printf
        if (fn->isVarArg() && llvm::isa<llvm::ConstantFP>(args[i]) &&
            args[i]->getType() != llvm::Type::getDoubleTy(*gen.ctx)) {
            args[i] = gen.builder->CreateFPExt(args[i], llvm::Type::getDoubleTy(*gen.ctx));
        }
    }
    return gen.builder->CreateCall(fn, args);
}

void FunctionCall::code_gen(Codegen &gen) {
    llvm_value(gen);
}

#endif

FunctionCall::FunctionCall(
        std::string name,
        std::vector<std::unique_ptr<Value>> values
) : name(std::move(name)), values(std::move(values)) {

}

void FunctionCall::link(ASTLinker &linker) {
    auto found = linker.current.find(name);
    if (found != linker.current.end()) {
        auto func = found->second->as_function();
        if (func != nullptr) {
            definition = func;
            for (const auto &value: values) {
                value->link(linker);
            }
        } else {
            linker.error("function call to identifier '" + name + "' is not valid, because its not a function.");
        }
    } else {
        linker.error("no function with name '" + name + "' found");
    }
}

ASTNode *FunctionCall::linked_node() {
    if (!definition) return nullptr;
    return definition->returnType->linked_node();
}

ASTNode *FunctionCall::find_link_in_parent(ASTNode *node) {
    auto found = node->child(name);
    auto func = found->as_function();
    if (func) {
        definition = func;
        return func;
    }
    return nullptr;
}

FunctionCall *FunctionCall::as_func_call() {
    return this;
}

bool FunctionCall::primitive() {
    return false;
}

Value *FunctionCall::find_in(InterpretScope &scope, Value *parent) {
    return parent->call_member(scope, name, values);
}

Value *FunctionCall::evaluated_value(InterpretScope &scope) {
    if (definition != nullptr) {
        return definition->call(&scope, values);
    } else {
        scope.error("(function call) calling a function that is not found or has no body, name : " + name);
    }
    return nullptr;
}

Value *FunctionCall::copy(InterpretScope &scope) {
    scope.error("copy called on function call");
    return nullptr;
}

Value *FunctionCall::initializer_value(InterpretScope &scope) {
    return evaluated_value(scope);
}

Value *FunctionCall::assignment_value(InterpretScope &scope) {
    return evaluated_value(scope);
}

Value *FunctionCall::param_value(InterpretScope &scope) {
    return evaluated_value(scope);
}

Value *FunctionCall::return_value(InterpretScope &scope) {
    return evaluated_value(scope);
}

void FunctionCall::interpret(InterpretScope &scope) {
    auto value = evaluated_value(scope);
    if (value != nullptr && value->primitive()) {
        delete value;
    }
}

std::string FunctionCall::representation() const {
    std::string rep;
    rep.append(name);
    rep.append(1, '(');
    int i = 0;
    while (i < values.size()) {
        rep.append(values[i]->representation());
        if (i != values.size() - 1) {
            rep.append(1, ',');
        }
        i++;
    }
    rep.append(");");
    return rep;
}