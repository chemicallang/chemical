// Copyright (c) Qinetik 2024.

#include "FunctionCall.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

std::vector<llvm::Value*> to_llvm_args(Codegen& gen, FunctionCall* call, std::vector<std::unique_ptr<Value>>& values, bool isVariadic) {
    std::vector<llvm::Value *> args(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        args[i] = values[i]->llvm_arg_value(gen, call, i);
        // Ensure proper type promotion for float values passed to printf
        if (isVariadic && llvm::isa<llvm::ConstantFP>(args[i]) &&
            args[i]->getType() != llvm::Type::getDoubleTy(*gen.ctx)) {
            args[i] = gen.builder->CreateFPExt(args[i], llvm::Type::getDoubleTy(*gen.ctx));
        }
    }
    return args;
}

llvm::Type *FunctionCall::llvm_type(Codegen &gen) {
    return linked->as_function()->returnType->llvm_type(gen);
}

llvm::Value *FunctionCall::llvm_value(Codegen &gen) {
    auto fn = gen.module->getFunction(name);
    if(fn != nullptr) {
        auto args = to_llvm_args(gen, this, values, fn->isVarArg());
        return gen.builder->CreateCall(fn, args);
    } else {
        // TODO hardcoded false to isVariadic
        auto args = to_llvm_args(gen, this, values, false);
        return gen.builder->CreateCall((llvm::FunctionType*) linked->llvm_type(gen), linked->llvm_pointer(gen), args);
    }
}

llvm::InvokeInst *FunctionCall::llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind) {
    auto fn = gen.module->getFunction(name);
    if(fn != nullptr) {
        auto args = to_llvm_args(gen, this, values, fn->isVarArg());
        return gen.builder->CreateInvoke(fn, normal, unwind, args);
    } else {
        gen.error("Unknown function call through invoke " + name);
        return nullptr;
    }
}

#endif

FunctionCall::FunctionCall(
        std::string name,
        std::vector<std::unique_ptr<Value>> values
) : name(std::move(name)), values(std::move(values)) {

}

void FunctionCall::link(SymbolResolver &linker) {
    auto found = linker.current.find(name);
    if (found != linker.current.end()) {
        linked = found->second;
        unsigned i = 0;
        while(i < values.size()) {
            values[i]->link(linker, this, i);
            i++;
        }
        if(found->second->as_function() == nullptr && !found->second->create_value_type()->satisfies(ValueType::Lambda)) {
            linker.error("function call to identifier '" + name + "' is not valid, because its not a function.");
        }
    } else {
        linker.error("no function with name '" + name + "' found");
    }
}

ASTNode *FunctionCall::linked_node() {
    if (!linked || !linked->as_function()) return nullptr;
    return linked->as_function()->returnType->linked_node();
}

ASTNode *FunctionCall::find_link_in_parent(ASTNode *node) {
    auto found = node->child(name);
    if (found) {
        linked = found;
        return linked;
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
    if (linked && linked->as_function()) {
        return linked->as_function()->call(&scope, values);
    } else {
        scope.error("(function call) calling a function that is not found or has no body, name : " + name);
    }
    return nullptr;
}

Value *FunctionCall::copy() {
    std::cerr << "copy called on function call" << std::endl;
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

std::unique_ptr<BaseType> FunctionCall::create_type() const {
    return linked->create_value_type();
}

llvm::Value * FunctionCall::llvm_pointer(Codegen &gen) {
    return linked->llvm_pointer(gen);
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
    rep.append(1, ')');
    return rep;
}