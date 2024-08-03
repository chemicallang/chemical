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
#include "ast/base/BaseType.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/utils/ASTUtils.h"

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
        unsigned int start,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
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
                args.emplace_back((*chain)[parent_index]->access_chain_value(gen, *chain, parent_index, destructibles));
            } else {
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

        if(values[i]->value_type() == ValueType::Struct) {
            destructibles.emplace_back(values[i].get(), argValue);
        }

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
    auto decl = safe_linked_func();
    int16_t prev_itr = set_curr_itr_on_decl();
    const auto type = get_base_type()->llvm_type(gen);
    decl->set_active_iteration_safely(prev_itr);
    return type;
}

llvm::Type *FunctionCall::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned int index) {
    return get_base_type()->llvm_chain_type(gen, values, index);
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
    AccessChain member_access(std::vector<std::unique_ptr<Value>> {}, nullptr, false);
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
        unsigned int until,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    std::vector<llvm::Value *> args;
    llvm::Value* value;
    if(chain && until > 1) {
        value = (*chain)[until - 1]->access_chain_value(gen, *chain, until - 1, destructibles);
    } else {
        value = call->parent_val->llvm_value(gen);
    };
    auto dataPtr = gen.builder->CreateStructGEP(gen.packed_lambda_type(), value, 1);
    auto data = gen.builder->CreateLoad(gen.builder->getPtrTy(), dataPtr);
    args.emplace_back(data);
    to_llvm_args(gen, call, func_type, call->values, args, chain, until, 0, destructibles);
    auto structType = gen.packed_lambda_type();
    auto lambdaPtr = gen.builder->CreateStructGEP(structType, value, 0);
    auto lambda = gen.builder->CreateLoad(gen.builder->getPtrTy(), lambdaPtr);
    return gen.builder->CreateCall(func_type->llvm_func_type(gen), lambda, args);
}

llvm::Value *FunctionCall::llvm_value(Codegen &gen, std::vector<llvm::Value*>& args) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    auto func_type = get_function_type();
    if(func_type->isCapturing) {
        return call_capturing_lambda(gen, this, func_type.get(), nullptr, 0, destructibles);
    }

    auto decl = safe_linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    to_llvm_args(gen, this, func_type.get(), values, args, nullptr, 0, args.size(), destructibles);

    // TODO handle destructibles here
    return call_with_args(this, fn, func_type.get(), gen,  args);
}

void FunctionCall::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    auto funcType = get_function_type();
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
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        llvm::Value* returnedStruct
) {

    auto func_type = get_function_type();
    if(func_type->isCapturing) {
        return call_capturing_lambda(gen, this, func_type.get(), &chain, until, destructibles);
    }

    auto decl = safe_linked_func();
    if(decl && !decl->generic_params.empty()) {
        decl->set_active_iteration(generic_iteration);
    }
    llvm::Value* returnedValue = returnedStruct;
    auto returnsStruct = func_type->returnType->value_type() == ValueType::Struct;

    if(decl && decl->has_annotation(AnnotationKind::CompTime)) {
        auto ret = std::unique_ptr<Value>(decl->call(&gen.comptime_scope, this, nullptr));
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
    to_llvm_args(gen, this, func_type.get(), values, args, &chain, until,0, destructibles);

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

llvm::Value* FunctionCall::access_chain_value(Codegen &gen, std::vector<std::unique_ptr<Value>> &chain, unsigned until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles) {
    std::vector<llvm::Value *> args;
    return llvm_chain_value(gen, args, chain, until, destructibles);
}

llvm::InvokeInst *FunctionCall::llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind) {
    auto decl = linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    if(fn != nullptr) {
        auto type = decl->create_value_type();
        std::vector<llvm::Value *> args;
        std::vector<std::pair<Value*, llvm::Value*>> destructibles;
        // TODO handle destructibles here
        to_llvm_args(gen, this, type->function_type(), values, args, nullptr, 0, 0, destructibles);
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
    auto func_type = get_function_type();
    if(func_type->returnType->value_type() == ValueType::Struct) {
        // we allocate the returned struct, llvm_chain_value function
        std::vector<llvm::Value *> args;
        std::vector<std::pair<Value*, llvm::Value*>> destructibles;
        // TODO handle destructibles here
        return (llvm::AllocaInst*) llvm_chain_value(gen, args, chain_values, until, destructibles);
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
    return get_base_type()->byte_size(is64Bit);
}

void FunctionCall::link_values(SymbolResolver &linker) {
    auto func_type = get_function_type();
    unsigned i = 0;
    while(i < values.size()) {
        values[i]->link(linker, this, i);
        const auto param = func_type->func_param_for_arg_at(i);
        if(param) {
            auto implicit_constructor = implicit_constructor_for(param->type.get(), values[i].get());
            if (implicit_constructor) {
                values[i] = call_with_arg(implicit_constructor, std::move(values[i]), linker);
            }
        }
        i++;
    }
}

void FunctionCall::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    link_values(linker);
}

std::unique_ptr<FunctionType> FunctionCall::create_function_type() {
    auto func_type = parent_val->create_type();
    return std::unique_ptr<FunctionType>((FunctionType*) func_type.release());
}

hybrid_ptr<FunctionType> FunctionCall::get_function_type() {
    auto decl = safe_linked_func();
    if(decl) {
        return hybrid_ptr<FunctionType> { create_function_type().release() };
    }
    auto func_type = parent_val->get_base_type();
    if(func_type->function_type()) {
        return hybrid_ptr<FunctionType>{(FunctionType *) func_type.release(), func_type.get_will_free()};
    } else {
        return hybrid_ptr<FunctionType>{nullptr, false };
    }
}

int16_t FunctionCall::set_curr_itr_on_decl() {
    int16_t prev_itr = -2;
    const auto decl = safe_linked_func();
    if(decl && !decl->generic_params.empty()) {
        prev_itr = decl->active_iteration;
        decl->set_active_iteration(generic_iteration);
    }
    return prev_itr;
}

ASTNode *FunctionCall::linked_node() {
    return get_base_type()->linked_node();
}

void FunctionCall::relink_multi_func(ASTDiagnoser* diagnoser) {
    auto parent = parent_val->as_identifier();
    if(parent) {
        if(parent->linked) {
            auto multi = parent->linked->as_multi_func_node();
            if(multi) {
                auto func = multi->func_for_call(values);
                if(func) {
                    parent->linked = func;
                } else {
                    diagnoser->error("couldn't find function with name " + parent->value + " that satisfies given arguments");
                }
            }
        }
    }
}

void FunctionCall::find_link_in_parent(Value *parent, ASTDiagnoser* diagnoser, bool relink_multi) {
    parent_val = parent;
    // relinking parent with constructor of the struct
    // if it's linked with struct
    auto parent_id = parent->as_identifier();
    if(parent_id && parent_id->linked && parent_id->linked->as_struct_def()) {
        StructDefinition* parent_struct = parent_id->linked->as_struct_def();
        auto constructorFunc = parent_struct->constructor_func(values);
        if(constructorFunc) {
            parent_id->linked = constructorFunc;
        } else {
            diagnoser->error("struct with name " + parent_struct->name + " doesn't have a constructor that satisfies given arguments " + representation(), parent_struct);
        }
    } else if(relink_multi){
        relink_multi_func(diagnoser);
    }
}

void FunctionCall::find_link_in_parent(Value *parent, SymbolResolver &resolver) {
    parent_val = parent;
    relink_multi_func(&resolver);
    FunctionDeclaration* func_decl = safe_linked_func();
    int16_t prev_itr;
    if(func_decl && !func_decl->generic_params.empty()) {
        prev_itr = func_decl->active_iteration;
        generic_iteration = func_decl->register_call(this);
        func_decl->set_active_iteration(generic_iteration);
    }
    find_link_in_parent(parent, &resolver, false);
    link_values(resolver);
    if(func_decl && !func_decl->generic_params.empty()) {
        func_decl->set_active_iteration(prev_itr);
    }
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
        return func->call(&scope, call, parent);
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

std::unique_ptr<Value> FunctionCall::create_evaluated_value(InterpretScope &scope) {
    return std::unique_ptr<Value>(scope_value(scope));
}

hybrid_ptr<Value> FunctionCall::evaluated_chain_value(InterpretScope &scope, Value* parent) {
    return hybrid_ptr<Value>{ interpret_value(this, scope, parent) };
}

void FunctionCall::evaluate_children(InterpretScope &scope) {
    evaluate_values(values, scope);
}

Value *FunctionCall::copy() {
    auto call = new FunctionCall({});
    for(auto& value : values) {
        call->values.emplace_back(value->copy());
    }
    call->parent_val = parent_val;
    return call;
}

std::unique_ptr<BaseType> FunctionCall::create_type() {
    auto prev_itr = set_curr_itr_on_decl();
    auto func_type = create_function_type();
    auto pure_type = func_type->returnType->get_pure_type();
    if(prev_itr >= -1) safe_linked_func()->set_active_iteration(prev_itr);
    return std::unique_ptr<BaseType>(pure_type->copy());
}

hybrid_ptr<BaseType> FunctionCall::get_base_type() {
    auto prev_itr = set_curr_itr_on_decl();
    auto func_type = get_function_type();
    auto pure_return = func_type->returnType->get_pure_type();
    if(prev_itr >= -1) safe_linked_func()->set_active_iteration(prev_itr);
    if(pure_return.get_will_free()) {
        return pure_return;
    } else {
        if(func_type.get_will_free()) {
            return hybrid_ptr<BaseType> { func_type->returnType.release(), true };
        } else {
            return hybrid_ptr<BaseType> { func_type->returnType.get(), false };
        }
    }
}

BaseTypeKind FunctionCall::type_kind() const {
    return const_cast<FunctionCall*>(this)->get_base_type()->kind();
}

ValueType FunctionCall::value_type() const {
    return const_cast<FunctionCall*>(this)->get_base_type()->value_type();
}

void FunctionCall::interpret(InterpretScope &scope) {
    evaluated_value(scope);
}