// Copyright (c) Qinetik 2024.

#include "ast/base/GlobalInterpretScope.h"

#ifdef COMPILER_BUILD

#include "ast/types/PointerType.h"
#include "compiler/llvmimpl.h"

llvm::Type *FunctionParam::llvm_elem_type(Codegen &gen) {
    auto lType = llvm_type(gen);
    if (lType) {
        if (lType->isArrayTy()) {
            return lType->getArrayElementType();
        } else if (lType->isPointerTy()) {
            auto ptr_type = type->pointer_type();
            if (ptr_type) {
                return ptr_type->type->llvm_type(gen);
            } else {
                gen.error("type is not a pointer type for parameter " + name);
            }
        } else {
            gen.error("type is not an array / pointer for parameter " + name);
        }
    } else {
        gen.error("parameter type is invalid " + name);
    }
    return nullptr;
}

llvm::Value *FunctionParam::llvm_pointer(Codegen &gen) {
    auto arg = gen.current_function->getArg(index);
    if (arg) {
        return arg;
    } else {
        gen.error("couldn't get argument with name " + name);
        return nullptr;
    }
}

llvm::FunctionType *FunctionDeclaration::function_type(Codegen &gen) {
    if (params.empty() || (params.size() == 1 && isVariadic)) {
        return llvm::FunctionType::get(returnType->llvm_type(gen), isVariadic);
    } else {
        return llvm::FunctionType::get(returnType->llvm_type(gen), param_types(gen), isVariadic);
    }
}

void FunctionDeclaration::code_gen(Codegen &gen) {
    if (body.has_value()) {
        declare(gen);
        gen.create_function(name, function_type(gen));
        body->code_gen(gen);
        undeclare(gen);
    } else {
        gen.declare_function(name, function_type(gen));
    }
}

#endif

void FunctionDeclaration::interpret_scope_ends(InterpretScope &scope) {
    scope.erase_node(name);
}

Value *FunctionDeclaration::call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_params,
                                 InterpretScope *fn_scope) {
    if (!body.has_value()) return nullptr;
    if (params.size() != call_params.size()) {
        fn_scope->error("function " + name + " requires " + std::to_string(params.size()) + ", but given params are " +
                        std::to_string(call_params.size()));
        return nullptr;
    }
    auto i = 0;
    while (i < params.size()) {
        fn_scope->declare(params[i].name, call_params[i]->param_value(*call_scope));
        i++;
    }
    auto previous = call_scope->global->curr_node_position;
    call_scope->global->curr_node_position = 0;
    body.value().interpret(*fn_scope);
    call_scope->global->curr_node_position = previous;
    // delete all the primitive values that were copied into the function
    i--;
    while (i > -1) {
        auto itr = fn_scope->find_value_iterator(params[i].name);
        if (itr.first != itr.second.end()) {
            if (itr.first->second != nullptr && itr.first->second->primitive()) {
                delete itr.first->second;
            }
            itr.second.erase(itr.first);
        } else {
            fn_scope->error("couldn't find parameter for cleanup after function call " + params[i].name);
        }
        i--;
    }
    return interpretReturn;
}