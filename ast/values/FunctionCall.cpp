// Copyright (c) Qinetik 2024.

#include "FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/LambdaFunction.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void to_llvm_args(Codegen& gen, FunctionCall* call, std::vector<std::unique_ptr<Value>>& values, bool isVariadic, std::vector<llvm::Value *>& args, unsigned int start = 0) {
    llvm::Value* argValue;
    auto linked_node = call->parent_val->linked_node();
    auto linked = call->parent_val->create_type();

    std::unique_ptr<FunctionType> expectedFunc = std::unique_ptr<FunctionType>((FunctionType*) linked.release());

    // if the called lambda is capturing, take argument next to lambda and pass it into it
    if (linked_node && expectedFunc->isCapturing && linked_node->as_func_param() != nullptr) {
        args.emplace_back(gen.current_function->getArg(linked_node->as_func_param()->index + 1));
    }

    for (size_t i = start; i < values.size(); ++i) {
        argValue = values[i]->llvm_arg_value(gen, call, i);
        // Ensure proper type promotion for float values passed to printf
        if (isVariadic && llvm::isa<llvm::ConstantFP>(argValue) &&
            argValue->getType() != llvm::Type::getDoubleTy(*gen.ctx)) {
            argValue = gen.builder->CreateFPExt(argValue, llvm::Type::getDoubleTy(*gen.ctx));
        }
        args.emplace_back(argValue);

        // expanding passed lambda values, to multiple (passing function pointer & also passing their struct so 1 arg results in 2 args)
        if(values[i]->value_type() == ValueType::Lambda) {
            auto expectedParam = expectedFunc->params[i]->create_value_type();
            auto expectedFuncType = (FunctionType*) expectedParam.get();
            auto type = values[i]->create_type();
            auto funcType = (FunctionType*) type.get();
            if(expectedFuncType->isCapturing) {
                if(funcType->isCapturing) {
                    if(values[i]->primitive()) {
                        args.emplace_back(((LambdaFunction*) values[i].get())->captured_struct);
                    } else {
                        auto lambda_linked = values[i]->linked_node();
                        if(lambda_linked->as_func_param() != nullptr) {
                            args.emplace_back(gen.current_function->getArg(lambda_linked->as_func_param()->index + 1));
                        } else {
                            throw std::runtime_error("unknown linked node to lambda referenced value");
                        }
                    }
                } else {
                    args.emplace_back(llvm::ConstantPointerNull::get(gen.builder->getPtrTy()));
                }
            }
        }
    }
}

inline std::vector<llvm::Value*> to_llvm_args(Codegen& gen, FunctionCall* call, std::vector<std::unique_ptr<Value>>& values, bool isVariadic) {
    std::vector<llvm::Value *> args;
    to_llvm_args(gen, call, values, isVariadic, args, 0);
    return args;
}

llvm::Type *FunctionCall::llvm_type(Codegen &gen) {
    return linked_func()->returnType->llvm_type(gen);
}

llvm::FunctionType *FunctionCall::llvm_func_type(Codegen &gen) {
    return linked_func()->returnType->llvm_func_type(gen);
}

llvm::Value* get_callee(Codegen &gen, FunctionCall* call) {
    llvm::Value* callee = nullptr;
    if(call->linked() != nullptr) {
        if(call->linked()->as_function() == nullptr) {
            callee = call->linked()->llvm_load(gen);
        } else {
            callee = call->linked()->llvm_pointer(gen);
        }
    } else {
        callee = call->parent_val->llvm_value(gen);
    }
    return callee;
}

llvm::Value* call_with_args(FunctionCall* call, llvm::Function* fn, Codegen &gen, std::vector<llvm::Value*>& args) {
    if(fn != nullptr) {
        return gen.builder->CreateCall(fn, args);
    } else {
        auto callee = get_callee(gen, call);
        if(callee == nullptr) {
            gen.error("Couldn't get callee value for the function call to " + call->representation());
            return nullptr;
        }
        return gen.builder->CreateCall(call->parent_val->llvm_func_type(gen), callee, args);
    }
}

llvm::Value *call_capturing_lambda(Codegen &gen, FunctionCall* call, std::unique_ptr<FunctionType>& func_type) {
    std::vector<llvm::Value *> args;
    auto value = call->parent_val->llvm_value(gen);
    auto dataPtr = gen.builder->CreateStructGEP(gen.packed_lambda_type(), value, 1);
    auto data = gen.builder->CreateLoad(gen.builder->getPtrTy(), dataPtr);
    args.emplace_back(data);
    auto decl = call->safe_linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    // TODO hardcoded isVarArg when can't get the function
    to_llvm_args(gen, call, call->values, fn != nullptr && fn->isVarArg(), args, 0);
    auto structType = gen.packed_lambda_type();
    auto lambdaPtr = gen.builder->CreateStructGEP(structType, value, 0);
    auto lambda = gen.builder->CreateLoad(gen.builder->getPtrTy(), lambdaPtr);
    return gen.builder->CreateCall(call->parent_val->llvm_func_type(gen), lambda, args);
}

llvm::Value *FunctionCall::llvm_value(Codegen &gen) {

    auto func_type = std::unique_ptr<FunctionType>((FunctionType*) parent_val->create_type().release());
    if(func_type->isCapturing && parent_val->linked_node() != nullptr && parent_val->linked_node()->as_var_init() != nullptr) {
        return call_capturing_lambda(gen, this, func_type);
    }

    std::vector<llvm::Value *> args;

    auto decl = safe_linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    // TODO hardcoded isVarArg when can't get the function
    to_llvm_args(gen, this, values, fn != nullptr && fn->isVarArg(), args, 0);

    return call_with_args(this, fn, gen,  args);
}

llvm::Value* FunctionCall::llvm_value(Codegen &gen, std::vector<std::unique_ptr<Value>>& chain) {

    auto func_type = std::unique_ptr<FunctionType>((FunctionType*) parent_val->create_type().release());
    if(func_type->isCapturing && parent_val->linked_node() != nullptr && parent_val->linked_node()->as_var_init() != nullptr) {
        return call_capturing_lambda(gen, this, func_type);
    }

    auto decl = safe_linked_func();
    auto requires_self = decl != nullptr && !decl->params.empty() && (decl->params[0]->name == "this" || decl->params[0]->name == "self");
    std::vector<llvm::Value *> args;

    // a pointer to parent
    if(requires_self) {
        args.emplace_back(chain[chain.size() - 3]->llvm_pointer(gen));
    }

    auto fn = decl != nullptr ? decl->llvm_func() : nullptr;
    // TODO hardcoded isVarArg when can't get the function
    to_llvm_args(gen, this, values, fn != nullptr && fn->isVarArg(), args, requires_self ? 1 :0);

    if(linked() && linked()->as_struct_member() != nullptr) { // means I'm calling a pointer inside a struct

        // creating access chain to the last member as an identifier instead of function call
        AccessChain member_access({});
        unsigned i = 0;
        while(i < (chain.size() - 1)) {
            member_access.values.emplace_back(chain[i]->copy());
            i++;
        }

        return gen.builder->CreateCall(linked()->llvm_func_type(gen), member_access.llvm_value(gen), args);

    }

    return call_with_args(this, fn, gen,  args);
}

llvm::InvokeInst *FunctionCall::llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind) {
    auto decl = linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    if(fn != nullptr) {
        auto args = to_llvm_args(gen, this, values, fn->isVarArg());
        return gen.builder->CreateInvoke(fn, normal, unwind, args);
    } else {
        gen.error("Unknown function call through invoke ");
        return nullptr;
    }
}

#endif

FunctionCall::FunctionCall(
        std::vector<std::unique_ptr<Value>> values
) : values(std::move(values)) {

}

void FunctionCall::link_values(SymbolResolver &linker) {
    unsigned i = 0;
    while(i < values.size()) {
        values[i]->link(linker, this, i);
        i++;
    }
}

void FunctionCall::link(SymbolResolver &linker) {
    throw std::runtime_error("cannot link a function call wihout identifier");
//    name->link(linker);
//    linked = name->linked_node();
//    if(linked) {
//        link_values(linker);
//        if(linked_func() == nullptr && !name->create_type()->satisfies(ValueType::Lambda)) {
//            linker.error("function call to identifier '" + name->representation() + "' is not valid, because its not a function.");
//        }
//    }
}

ASTNode *FunctionCall::linked_node() {
    auto func_type = parent_val->create_type();
    return ((FunctionType*) func_type.get())->returnType->linked_node();
}

void FunctionCall::find_link_in_parent(Value *parent, SymbolResolver &resolver) {
    parent_val = parent;
    link_values(resolver);
}

FunctionCall *FunctionCall::as_func_call() {
    return this;
}

bool FunctionCall::primitive() {
    return false;
}

Value *FunctionCall::find_in(InterpretScope &scope, Value *parent) {
    auto id = parent->as_identifier();
    if(id != nullptr) {
        return parent->call_member(scope, id->value, values);
    } else {
        scope.error("No identifier for function call");
    }
}

Value *FunctionCall::evaluated_value(InterpretScope &scope) {
    if (safe_linked_func()) {
        return linked_func()->call(&scope, values);
    } else {
        scope.error("(function call) calling a function that is not found or has no body");
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
    auto value_type = parent_val->create_type();
    auto func_type = value_type->function_type();
    return std::unique_ptr<BaseType>(func_type->returnType->copy());
}

llvm::Value *FunctionCall::llvm_pointer(Codegen &gen) {
    throw std::runtime_error("llvm_pointer called on a function call");
}

void FunctionCall::interpret(InterpretScope &scope) {
    auto value = evaluated_value(scope);
    if (value != nullptr && value->primitive()) {
        delete value;
    }
}

std::string FunctionCall::representation() const {
    std::string rep;
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