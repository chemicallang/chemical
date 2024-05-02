// Copyright (c) Qinetik 2024.

#include "FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void to_llvm_args(Codegen& gen, FunctionCall* call, std::vector<std::unique_ptr<Value>>& values, bool isVariadic, std::vector<llvm::Value *>& args, unsigned int start = 0) {
    for (size_t i = start; i < values.size(); ++i) {
        args[i] = values[i]->llvm_arg_value(gen, call, i);
        // Ensure proper type promotion for float values passed to printf
        if (isVariadic && llvm::isa<llvm::ConstantFP>(args[i]) &&
            args[i]->getType() != llvm::Type::getDoubleTy(*gen.ctx)) {
            args[i] = gen.builder->CreateFPExt(args[i], llvm::Type::getDoubleTy(*gen.ctx));
        }
    }
}

inline std::vector<llvm::Value*> to_llvm_args(Codegen& gen, FunctionCall* call, std::vector<std::unique_ptr<Value>>& values, bool isVariadic) {
    std::vector<llvm::Value *> args(values.size());
    to_llvm_args(gen, call, values, isVariadic, args, 0);
    return args;
}

llvm::Type *FunctionCall::llvm_type(Codegen &gen) {
    return linked->as_function()->returnType->llvm_type(gen);
}

llvm::Value* call_with_args(FunctionCall* call, llvm::Function* fn, Codegen &gen, std::vector<llvm::Value*>& args) {
    if(fn != nullptr) {
        return gen.builder->CreateCall(fn, args);
    } else {
        auto callee = call->linked->as_var_init() != nullptr ? call->linked->llvm_load(gen) : call->linked->llvm_pointer(gen);
        return gen.builder->CreateCall(call->linked->llvm_func_type(gen), callee, args);
    }
}

llvm::Value *FunctionCall::llvm_value(Codegen &gen) {
    std::vector<llvm::Value *> args(values.size());

    auto decl = linked->as_function();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    // TODO hardcoded isVarArg when can't get the function
    to_llvm_args(gen, this, values, fn != nullptr && fn->isVarArg(), args, 0);

    return call_with_args(this, fn, gen,  args);
}

llvm::Value* FunctionCall::llvm_value(Codegen &gen, std::vector<std::unique_ptr<Value>>& chain) {
    auto decl = linked->as_function();
    auto requires_self = decl != nullptr && !decl->params.empty() && (decl->params[0]->name == "this" || decl->params[0]->name == "self");
    std::vector<llvm::Value *> args(values.size() + (requires_self ? 1 : 0));

    // a pointer to parent
    if(requires_self) {
        args[0] = chain[chain.size() - 2]->llvm_pointer(gen);
    }

    auto fn = decl != nullptr ? decl->llvm_func() : nullptr;
    // TODO hardcoded isVarArg when can't get the function
    to_llvm_args(gen, this, values, fn != nullptr && fn->isVarArg(), args, requires_self ? 1 :0);

    if(linked->as_struct_member() != nullptr) { // means I'm calling a pointer inside a struct

        // creating access chain to the last member as an identifier instead of function call
        AccessChain member_access({});
        unsigned i = 0;
        while(i < (chain.size() - 1)) {
            member_access.values.emplace_back(chain[i]->copy());
            i++;
        }
//        auto identifier = new VariableIdentifier(name);
//        identifier->linked = linked;
        member_access.values.emplace_back(name->copy());

        return gen.builder->CreateCall(linked->llvm_func_type(gen), member_access.llvm_value(gen), args);

    }

    return call_with_args(this, fn, gen,  args);
}

llvm::InvokeInst *FunctionCall::llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind) {
    auto decl = linked->as_function();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    if(fn != nullptr) {
        auto args = to_llvm_args(gen, this, values, fn->isVarArg());
        return gen.builder->CreateInvoke(fn, normal, unwind, args);
    } else {
        gen.error("Unknown function call through invoke " + name->representation());
        return nullptr;
    }
}

#endif

FunctionCall::FunctionCall(
        std::unique_ptr<Value> identifier,
        std::vector<std::unique_ptr<Value>> values
) : name(std::move(identifier)), values(std::move(values)) {

}

void FunctionCall::link(SymbolResolver &linker) {
    name->link(linker);
    linked = name->linked_node();
    if(linked) {
        unsigned i = 0;
        while(i < values.size()) {
            values[i]->link(linker, this, i);
            i++;
        }
        if(linked->as_function() == nullptr && !linked->create_value_type()->satisfies(ValueType::Lambda)) {
            linker.error("function call to identifier '" + name->representation() + "' is not valid, because its not a function.");
        }
    } else {
        linker.error("no function with name '" + name->representation() + "' found");
    }
}

ASTNode *FunctionCall::linked_node() {
    if (!linked || !linked->as_function()) return nullptr;
    return linked->as_function()->returnType->linked_node();
}

ASTNode *FunctionCall::find_link_in_parent(ASTNode *node) {
    name->find_link_in_parent(node);
    auto found = name->linked_node();
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
    auto id = name->as_identifier();
    if(id != nullptr) {
        return parent->call_member(scope, id->value, values);
    } else {
        scope.error("No identifier for function call");
    }
}

Value *FunctionCall::evaluated_value(InterpretScope &scope) {
    if (linked && linked->as_function()) {
        return linked->as_function()->call(&scope, values);
    } else {
        scope.error("(function call) calling a function that is not found or has no body, name : " + name->representation());
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
    std::string rep(name->representation());
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