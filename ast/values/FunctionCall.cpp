// Copyright (c) Qinetik 2024.

#include "FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/LambdaFunction.h"
#include "ast/utils/ASTUtils.h"
#include "ast/structures/StructDefinition.h"
#include "compiler/SymbolResolver.h"
#include "ast/values/StructValue.h"

inline std::unique_ptr<FunctionType> func_call_func_type(const FunctionCall* call) {
    return std::unique_ptr<FunctionType>((FunctionType*) call->parent_val->create_type().release());
}

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void to_llvm_args(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<std::unique_ptr<Value>>& values,
        std::vector<llvm::Value *>& args,
        std::vector<std::unique_ptr<Value>>* chain,
        unsigned int until,
        unsigned int start
) {

    llvm::Value* argValue;

    // check function doesn't require a 'self' argument
    auto self_param = func_type->get_self_param();
    if(chain && self_param) {
        // a pointer to parent
        if (chain_contains_func_call(*chain, 0, chain->size() - 3)) {
            gen.error("cannot pass self when access chain has a function call");
            return;
        }
        int parent_index = (int) until - 2;
        if (parent_index >= 0 && parent_index < chain->size()) {
            if ((*chain)[parent_index]->value_type() == ValueType::Pointer) {
                args.emplace_back((*chain)[parent_index]->access_chain_value(gen, *chain, parent_index));
            } else {
                std::vector<std::pair<Value *, llvm::Value *>> destructibles;
                args.emplace_back((*chain)[parent_index]->access_chain_pointer(gen, *chain, destructibles, parent_index));
            }
        } else if(gen.current_func_type) {
            auto passing_self_arg = gen.current_func_type->get_self_param();
            if(passing_self_arg && passing_self_arg->type->is_same(self_param->type.get())) {
                args.emplace_back(passing_self_arg->llvm_load(gen));
            } else {
                gen.error("function without a self argument cannot call methods that require self arg");
                return;
            }
        }
    }

    for (size_t i = start; i < values.size(); ++i) {
        argValue = values[i]->llvm_arg_value(gen, call, i);

        // Ensure proper type promotion for float values passed to printf
        if (func_type->isVariadic && func_type->isInVarArgs(i) && argValue->getType()->isFloatTy()) {
            argValue = gen.builder->CreateFPExt(argValue, llvm::Type::getDoubleTy(*gen.ctx));
        }
        args.emplace_back(argValue);

        // expanding passed lambda values, to multiple (passing function pointer & also passing their struct so 1 arg results in 2 args)
//        if(values[i]->value_type() == ValueType::Lambda) {
//            auto expectedParam = func_type->params[i]->create_value_type();
//            auto expectedFuncType = (FunctionType*) expectedParam.get();
//            auto type = values[i]->create_type();
//            auto funcType = (FunctionType*) type.get();
//            if(expectedFuncType->isCapturing) {
//                if(funcType->isCapturing) {
//                    if(values[i]->primitive()) {
//                        args.emplace_back(((LambdaFunction*) values[i].get())->captured_struct);
//                    } else {
//                        auto lambda_linked = values[i]->linked_node();
//                        if(lambda_linked->as_func_param() != nullptr) {
//                            args.emplace_back(gen.current_function->getArg(lambda_linked->as_func_param()->index + 1));
//                        } else {
//                            throw std::runtime_error("unknown linked node to lambda referenced value");
//                        }
//                    }
//                } else {
//                    args.emplace_back(llvm::ConstantPointerNull::get(gen.builder->getPtrTy()));
//                }
//            }
//        }
    }
}

llvm::Type *FunctionCall::llvm_type(Codegen &gen) {
    return func_call_func_type(this)->returnType->llvm_type(gen);
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

llvm::Value* call_with_args(FunctionCall* call, llvm::Function* fn, FunctionType* func_type, Codegen &gen, std::vector<llvm::Value*>& args) {
    if(fn != nullptr) {
        return gen.builder->CreateCall(fn, args);
    } else {
        auto callee = get_callee(gen, call);
        if(callee == nullptr) {
            gen.error("Couldn't get callee value for the function call to " + call->representation());
            return nullptr;
        }
        return gen.builder->CreateCall(func_type->llvm_func_type(gen), callee, args);
    }
}

AccessChain parent_chain(FunctionCall* call, std::vector<std::unique_ptr<Value>>& chain, int till) {
    AccessChain member_access(std::vector<std::unique_ptr<Value>> {}, nullptr);
    unsigned i = 0;
    while(i < till) {
        if(chain[i].get() == call) {
            break;
        }
        member_access.values.emplace_back(chain[i]->copy());
        i++;
    }
    return member_access;
}

AccessChain parent_chain(FunctionCall* call, std::vector<std::unique_ptr<Value>>& chain) {
    return parent_chain(call, chain, chain.size() - 1);
}

AccessChain grandparent_chain(FunctionCall* call, std::vector<std::unique_ptr<Value>>& chain) {
    return parent_chain(call, chain, chain.size() - 2);
}

llvm::Value *call_capturing_lambda(
        Codegen &gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<std::unique_ptr<Value>>* chain,
        unsigned int until
) {
    std::vector<llvm::Value *> args;
    llvm::Value* value;
    if(chain && until > 1) {
        value = (*chain)[until - 1]->access_chain_value(gen, *chain, until - 1);
    } else {
        value = call->parent_val->llvm_value(gen);
    };
    auto dataPtr = gen.builder->CreateStructGEP(gen.packed_lambda_type(), value, 1);
    auto data = gen.builder->CreateLoad(gen.builder->getPtrTy(), dataPtr);
    args.emplace_back(data);
    to_llvm_args(gen, call, func_type, call->values, args, chain, until, 0);
    auto structType = gen.packed_lambda_type();
    auto lambdaPtr = gen.builder->CreateStructGEP(structType, value, 0);
    auto lambda = gen.builder->CreateLoad(gen.builder->getPtrTy(), lambdaPtr);
    return gen.builder->CreateCall(func_type->llvm_func_type(gen), lambda, args);
}

llvm::Value *FunctionCall::llvm_value(Codegen &gen, std::vector<llvm::Value*>& args) {
    auto func_type = func_call_func_type(this);
    if(func_type->isCapturing) {
        return call_capturing_lambda(gen, this, func_type.get(), nullptr, 0);
    }

    auto decl = safe_linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    to_llvm_args(gen, this, func_type.get(), values, args, nullptr, 0, args.size());

    return call_with_args(this, fn, func_type.get(), gen,  args);
}

void FunctionCall::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    auto funcType = func_call_func_type(this);
    auto linked = funcType->returnType->linked_node();
    if(linked) {
        linked->llvm_destruct(gen, allocaInst);
    }
}

llvm::Value *FunctionCall::llvm_value(Codegen &gen) {
    std::vector<llvm::Value *> args;
    return llvm_value(gen, args);
}

llvm::Value* FunctionCall::llvm_chain_value(
        Codegen &gen,
        std::vector<llvm::Value*>& args,
        std::vector<std::unique_ptr<Value>>& chain,
        unsigned int until,
        llvm::Value* returnedStruct
) {

    auto func_type = func_call_func_type(this);
    if(func_type->isCapturing) {
        return call_capturing_lambda(gen, this, func_type.get(), &chain, until);
    }

    auto decl = safe_linked_func();
    llvm::Value* returnedValue = returnedStruct;
    auto returnsStruct = func_type->returnType->value_type() == ValueType::Struct;

    if(decl && decl->has_annotation(AnnotationKind::CompTime)) {
        auto ret = std::unique_ptr<Value>(decl->call(&gen.comptime_scope, values, nullptr));
        auto val = ret->evaluated_value(gen.comptime_scope);
        if(!val) {
            gen.error("compile time function didn't return a value");
            return nullptr;
        }
        auto as_struct = val->as_struct();
        if(as_struct) {
            if(!returnedStruct) {
                returnedValue = gen.builder->CreateAlloca(func_type->returnType->llvm_type(gen), nullptr);
            }
            as_struct->initialize_alloca((llvm::AllocaInst*) returnedValue, gen);
            return returnedValue;
        } else {
            return val->llvm_value(gen);
        }
    }

    if(returnsStruct) {
        if(!returnedStruct) {
            returnedValue = gen.builder->CreateAlloca(func_type->returnType->llvm_type(gen), nullptr);
        }
        args.emplace_back(returnedValue);
    }

    auto fn = decl != nullptr ? decl->llvm_func() : nullptr;
    to_llvm_args(gen, this, func_type.get(), values, args, &chain, until,0);

    llvm::Value* call_value;

    if(linked() && linked()->as_struct_member() != nullptr) { // means I'm calling a pointer inside a struct

        // creating access chain to the last member as an identifier instead of function call
        auto parent_access = parent_chain(this, chain);

        call_value = gen.builder->CreateCall(linked()->llvm_func_type(gen), parent_access.llvm_value(gen), args);

    } else {
        call_value = call_with_args(this, fn, func_type.get(), gen, args);
    }

    return returnedValue ? returnedValue : call_value;

}

llvm::Value* FunctionCall::access_chain_value(Codegen &gen, std::vector<std::unique_ptr<Value>> &chain, unsigned until) {
    std::vector<llvm::Value *> args;
    return llvm_chain_value(gen, args, chain, until);
}

llvm::InvokeInst *FunctionCall::llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind) {
    auto decl = linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    if(fn != nullptr) {
        auto type = decl->create_value_type();
        std::vector<llvm::Value *> args;
        to_llvm_args(gen, this, type->function_type(), values, args, nullptr, 0, 0);
        return gen.builder->CreateInvoke(fn, normal, unwind, args);
    } else {
        gen.error("Unknown function call through invoke ");
        return nullptr;
    }
}

llvm::Value *FunctionCall::llvm_pointer(Codegen &gen) {
    throw std::runtime_error("llvm_pointer called on a function call");
}

bool FunctionCall::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return create_type()->linked_node()->add_child_index(gen, indexes, name);
}

llvm::AllocaInst *FunctionCall::access_chain_allocate(Codegen &gen, std::vector<std::unique_ptr<Value>> &chain_values, unsigned int until) {
    auto func_type = func_call_func_type(this);
    if(func_type->returnType->value_type() == ValueType::Struct) {
        // we allocate the returned struct, llvm_chain_value function
        std::vector<llvm::Value *> args;
        return (llvm::AllocaInst*) llvm_chain_value(gen, args, chain_values, until);
    } else {
        return Value::access_chain_allocate(gen, chain_values, until);
    }
}

#endif

FunctionCall::FunctionCall(
        std::vector<std::unique_ptr<Value>> values
) : values(std::move(values)) {

}

uint64_t FunctionCall::byte_size(bool is64Bit) {
    return func_call_func_type(this)->returnType->byte_size(is64Bit);
}

void FunctionCall::link_values(SymbolResolver &linker) {
    unsigned i = 0;
    while(i < values.size()) {
        values[i]->link(linker, this, i);
        i++;
    }
}

void FunctionCall::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    throw std::runtime_error("cannot link a function call without identifier");
//    name->link(linker);
//    linked = name->linked_node();
//    if(linked) {
//        link_values(linker);
//        if(linked_func() == nullptr && !name->create_type()->satisfies(ValueType::Lambda)) {
//            linker.error("function call to identifier '" + name->representation() + "' is not valid, because its not a function.");
//        }
//    }
}

std::unique_ptr<FunctionType> FunctionCall::create_function_type() {
    auto func_type = parent_val->create_type();
    return std::unique_ptr<FunctionType>((FunctionType*) func_type.release());
}

ASTNode *FunctionCall::linked_node() {
    auto func_type = parent_val->create_type();
    return ((FunctionType*) func_type.get())->returnType->linked_node();
}

void FunctionCall::find_link_in_parent(Value *parent, SymbolResolver &resolver) {
    parent_val = parent;
    // relinking parent with constructor of the struct
    // if it's linked with struct
    auto parent_id = parent->as_identifier();
    auto parent_linked = parent_val->linked_node();
    if(parent_id && parent_linked && parent_linked->as_struct_def()) {
        StructDefinition* parent_struct = parent_linked->as_struct_def();
        auto constructorFunc = parent_struct->constructor_func(values);
        if(constructorFunc) {
            parent_id->linked = constructorFunc;
        } else {
            resolver.error("struct with name " + parent_struct->name + " doesn't have a constructor that satisfies given arguments " + representation(), parent_struct);
        }
    }
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
        return nullptr;
    }
}

Value* interpret_value(FunctionCall* call, InterpretScope &scope, Value* parent) {
    auto func = call->safe_linked_func();
    if (func) {
        return func->call(&scope, call->values, parent);
    } else {
        scope.error("(function call) calling a function that is not found or has no body");
    }
    return nullptr;
}

Value *FunctionCall::scope_value(InterpretScope &scope) {
    return interpret_value(this, scope, nullptr);
}

hybrid_ptr<Value> FunctionCall::evaluated_value(InterpretScope &scope) {
    return hybrid_ptr<Value> {scope_value(scope) };
}

hybrid_ptr<Value> FunctionCall::evaluated_chain_value(InterpretScope &scope, hybrid_ptr<Value> &parent) {
    auto value = interpret_value(this, scope, parent.get());
    return hybrid_ptr<Value>{ value };
}

void FunctionCall::evaluate_children(InterpretScope &scope) {
    evaluate_values(values, scope);
}

Value *FunctionCall::copy() {
    std::cerr << "copy called on function call" << std::endl;
    return nullptr;
}

std::unique_ptr<BaseType> FunctionCall::create_type() {
    auto value_type = parent_val->create_type();
    auto func_type = value_type->function_type();
    return std::unique_ptr<BaseType>(func_type->returnType->copy());
}

void FunctionCall::interpret(InterpretScope &scope) {
    evaluated_value(scope);
}