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
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void put_self_param(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<std::unique_ptr<Value>>& values,
        std::vector<llvm::Value *>& args,
        std::vector<std::unique_ptr<ChainValue>>* chain,
        unsigned int until,
        unsigned int start,
        llvm::Value* self_arg_val,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    // check function doesn't require a 'self' argument
    auto self_param = func_type->get_self_param();
    if(chain && self_param) {
        // a pointer to parent
        if (chain_contains_func_call(*chain, 0, chain->size() - 3)) {
            gen.error("cannot pass self when access chain has a function call", call);
            return;
        }
        int parent_index = (int) until - 2;
        if (parent_index >= 0 && parent_index < chain->size()) {
            // TODO do not load chain until
            if(!self_arg_val) {
                self_arg_val = llvm_load_chain_until(gen, *chain, parent_index, destructibles);
            }
            args.emplace_back(self_arg_val);
        } else if(gen.current_func_type) {
            auto passing_self_arg = gen.current_func_type->get_self_param();
            if(passing_self_arg && passing_self_arg->type->is_same(self_param->type.get())) {
                args.emplace_back(passing_self_arg->llvm_load(gen));
            } else {
                gen.error("function without a self argument cannot call methods that require self arg", call);
                return;
            }
        }
    }
}

void to_llvm_args(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<std::unique_ptr<Value>>& values,
        std::vector<llvm::Value *>& args,
        std::vector<std::unique_ptr<ChainValue>>* chain,
        unsigned int until,
        unsigned int start
) {

    llvm::Value* argValue;

    for (size_t i = start; i < values.size(); ++i) {

        const auto func_param = func_type->func_param_for_arg_at(i);
        if((func_param->type->get_direct_linked_struct() || func_param->type->get_direct_linked_variant()) && (values[i]->reference() && values[i]->value_type() == ValueType::Struct) && !(values[i]->as_struct() || values[i]->as_array_value() || values[i]->as_variant_call())) {
            argValue = values[i]->llvm_pointer(gen);
        } else {
            argValue = values[i]->llvm_arg_value(gen, call, i);
        }

//        if(values[i]->value_type() == ValueType::Struct) {
//            destructibles.emplace_back(values[i].get(), argValue);
//        }

        // Ensure proper type promotion for float values passed to printf
        if (func_type->isVariadic && func_type->isInVarArgs(i) && argValue->getType()->isFloatTy()) {
            argValue = gen.builder->CreateFPExt(argValue, llvm::Type::getDoubleTy(*gen.ctx));
        } else {
            argValue = gen.pack_dyn_obj(values[i].get(), func_param->type.get(), argValue);
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

void to_llvm_args(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<std::unique_ptr<Value>>& values,
        std::vector<llvm::Value *>& args,
        std::vector<std::unique_ptr<ChainValue>>* chain,
        unsigned int until,
        unsigned int start,
        llvm::Value* self_arg_val,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    put_self_param(gen, call, func_type, values, args, chain, until, start, self_arg_val, destructibles);
    to_llvm_args(gen, call, func_type, values, args, chain, until, start);
}

llvm::Type *FunctionCall::llvm_type(Codegen &gen) {
    auto decl = safe_linked_func();
    int16_t prev_itr = set_curr_itr_on_decl();
    const auto type = get_base_type()->llvm_type(gen);
    decl->set_active_iteration_safely(prev_itr);
    return type;
}

llvm::Type *FunctionCall::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    return get_base_type()->llvm_chain_type(gen, values, index);
}

llvm::FunctionType *FunctionCall::llvm_func_type(Codegen &gen) {
    return linked_func()->returnType->llvm_func_type(gen);
}

std::pair<llvm::Value*, llvm::FunctionType*>* FunctionCall::llvm_generic_func_data(std::vector<std::unique_ptr<ChainValue>> &chain_values, unsigned int index) {
    auto gen_str = get_grandpa_generic_struct(chain_values, index);
    if(gen_str.first) {
        return &gen_str.first->llvm_generic_func_data(linked_func(), gen_str.second, generic_iteration);
    }
    return nullptr;
}

llvm::FunctionType *FunctionCall::llvm_linked_func_type(Codegen& gen, std::vector<std::unique_ptr<ChainValue>> &chain_values, unsigned int index) {
    const auto generic_data = llvm_generic_func_data(chain_values, index);
    if(generic_data) {
        return generic_data->second;
    }
    return get_function_type()->llvm_func_type(gen);
}

llvm::Value* call_with_callee(
        FunctionCall* call,
        Codegen &gen,
        std::vector<llvm::Value*>& args,
        std::vector<std::unique_ptr<ChainValue>> &chain_values,
        unsigned int index,
        llvm::Value* callee
) {
    const auto llvm_func_type = call->llvm_linked_func_type(gen, chain_values, index);
    return gen.builder->CreateCall(llvm_func_type, callee, args);
}

llvm::Value* struct_return_in_args(
        Codegen& gen,
        std::vector<llvm::Value*>& args,
        FunctionType* func_type,
        llvm::Value* returnedValue
) {
    if(func_type->returnType->value_type() == ValueType::Struct) {
        if(!returnedValue) {
            returnedValue = gen.builder->CreateAlloca(func_type->returnType->llvm_type(gen), nullptr);
        }
        args.emplace_back(returnedValue);
    }
    return returnedValue;
}

std::pair<bool, llvm::Value*> FunctionCall::llvm_dynamic_dispatch(
        Codegen& gen,
        std::vector<std::unique_ptr<ChainValue>> &chain_values,
        unsigned int index,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    auto grandpa = get_grandpa_value(chain_values, index);
    if(!grandpa) return { false, nullptr };
    const auto known_t = grandpa->known_type();
    if(!known_t) return { false, nullptr };
    const auto pure_type = known_t->pure_type();
    if(pure_type->kind() != BaseTypeKind::Dynamic) return { false, nullptr };
    const auto linked = safe_linked_func();
    const auto interface = ((DynamicType*) pure_type)->referenced->linked_node()->as_interface_def();
    // got a pointer to the object it is being called upon, this reference points to a dynamic object (two pointers)
    auto granny = grandpa->access_chain_pointer(gen, chain_values, destructibles, index - 2);
    const auto struct_ty = gen.fat_pointer_type();

    auto func_type = get_function_type();
    llvm::Value* self_ptr = nullptr;

    if(func_type->has_self_param()) {
        // the pointer to the struct object present in dynamic object (must be loaded)
        const auto first_ele_ptr = gen.builder->CreateGEP(struct_ty, granny, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
        self_ptr = gen.builder->CreateLoad(gen.builder->getPtrTy(), first_ele_ptr);
    }

    // the pointer to implementation (vtable) we stored for the given interface (must be loaded)
    const auto second_ele_ptr = gen.builder->CreateGEP(struct_ty, granny, { gen.builder->getInt32(0), gen.builder->getInt32(1) }, "", gen.inbounds);
    const auto second_ele = gen.builder->CreateLoad(gen.builder->getPtrTy(), second_ele_ptr);
    // getting the index of the pointer stored in vtable using the interface and function
    const int func_index = interface->vtable_function_index(linked);
    // loading the pointer to the function, with GEP we are doing pointer math to find the correct function
    llvm::Value* callee_ptr = second_ele;
    if(func_index != 0) { // at zero we can just call second_ele since it's the first func pointer
        callee_ptr = gen.builder->CreateGEP(interface->llvm_vtable_type(gen), second_ele, { gen.builder->getInt32(func_index) }, "", gen.inbounds);;
    }
    // load the actual function pointer
    llvm::Value* callee = gen.builder->CreateLoad(gen.builder->getPtrTy(), callee_ptr);

    // we must use callee to call the function,
    std::vector<llvm::Value*> args;
    llvm::Value* returned_value = struct_return_in_args(gen, args, func_type.get(), nullptr);
    to_llvm_args(gen, this, func_type.get(), values, args, &chain_values, index, 0, self_ptr, destructibles);

    llvm::Value* call_value = call_with_callee(this, gen, args, chain_values, index, callee);
    return { true, returned_value ? returned_value : call_value };

}

llvm::Value *FunctionCall::llvm_linked_func_callee(
        Codegen& gen,
        std::vector<std::unique_ptr<ChainValue>> &chain_values,
        unsigned int index,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    const auto generic_data = llvm_generic_func_data(chain_values, index);
    if(generic_data) {
        return generic_data->first;
    }
    llvm::Value* callee = nullptr;
    if(linked() != nullptr) {
        if(linked()->as_function() == nullptr) {
            callee = linked()->llvm_load(gen);
        } else {
            callee = linked()->llvm_pointer(gen);
        }
    } else {
        callee = parent_val->access_chain_value(gen, chain_values, index - 1, destructibles, nullptr);
    }
    return callee;
}

llvm::Value* call_with_args(
        FunctionCall* call,
        llvm::Function* fn,
        Codegen &gen,
        std::vector<llvm::Value*>& args,
        std::vector<std::unique_ptr<ChainValue>> &chain_values,
        unsigned int index,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
//    if(fn != nullptr) {
//        return gen.builder->CreateCall(fn, args);
//    } else {
        auto callee = call->llvm_linked_func_callee(gen, chain_values, index, destructibles);
        if(callee == nullptr) {
            gen.error("Couldn't get callee value for the function call to " + call->representation(), call);
            return nullptr;
        }
        const auto llvm_func_type = call->llvm_linked_func_type(gen, chain_values, index);
        return gen.builder->CreateCall(llvm_func_type, callee, args);
//    }
}

AccessChain parent_chain(FunctionCall* call, std::vector<std::unique_ptr<ChainValue>>& chain, int till) {
    AccessChain member_access(std::vector<std::unique_ptr<ChainValue>> {}, nullptr, false, nullptr);
    unsigned i = 0;
    while(i < till) {
        if(chain[i].get() == call) {
            break;
        }
        member_access.values.emplace_back((ChainValue*)chain[i]->copy());
        i++;
    }
    return member_access;
}

AccessChain parent_chain(FunctionCall* call, std::vector<std::unique_ptr<ChainValue>>& chain) {
    return parent_chain(call, chain, chain.size() - 1);
}

AccessChain grandparent_chain(FunctionCall* call, std::vector<std::unique_ptr<ChainValue>>& chain) {
    return parent_chain(call, chain, chain.size() - 2);
}

llvm::Value *call_capturing_lambda(
        Codegen &gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<std::unique_ptr<ChainValue>>* chain,
        unsigned int until,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    llvm::Value* value;
    if(until > 1) {
        value = (*chain)[until - 1]->access_chain_value(gen, *chain, until - 1, destructibles, nullptr);
    } else {
        value = call->parent_val->llvm_value(gen);
    };
    auto dataPtr = gen.builder->CreateStructGEP(gen.fat_pointer_type(), value, 1);
    auto data = gen.builder->CreateLoad(gen.builder->getPtrTy(), dataPtr);
    std::vector<llvm::Value *> args;
    put_self_param(gen, call, func_type, call->values, args, chain, until, 0, nullptr, destructibles);
    args.emplace_back(data);
    to_llvm_args(gen, call, func_type, call->values, args, chain, until, 0);
    auto structType = gen.fat_pointer_type();
    auto lambdaPtr = gen.builder->CreateStructGEP(structType, value, 0);
    auto lambda = gen.builder->CreateLoad(gen.builder->getPtrTy(), lambdaPtr);
    return gen.builder->CreateCall(func_type->llvm_func_type(gen), lambda, args);
}

void FunctionCall::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    const auto func = safe_linked_func();
    if(func && func->has_annotation(AnnotationKind::CompTime)) {
        auto eval = gen.evaluated_func_calls.find(this);
        if(eval != gen.evaluated_func_calls.end()) {
            eval->second->llvm_destruct(gen, allocaInst);
            return;
        } else {
            // should this be reported ?
//            gen.info("couldn't find evaluated value of the function to destruct");
        }
    }
    auto funcType = get_function_type();
    auto linked = funcType->returnType->linked_node();
    if(linked) {
        int16_t prev_itr;
        const auto struct_def = linked->as_struct_def();
        if(struct_def && struct_def->is_generic()) {
            prev_itr = struct_def->active_iteration;
            struct_def->set_active_iteration(funcType->returnType->get_generic_iteration());
        }
        linked->llvm_destruct(gen, allocaInst);
        if(struct_def && struct_def->is_generic()) {
            struct_def->set_active_iteration(prev_itr);
        }
    }
}

llvm::Value* FunctionCall::llvm_chain_value(
        Codegen &gen,
        std::vector<llvm::Value*>& args,
        std::vector<std::unique_ptr<ChainValue>>& chain,
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
        auto& val = gen.eval_comptime(this, decl);
        if(!val) {
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

    auto dynamic_dispatch = llvm_dynamic_dispatch(gen, chain, until, destructibles);
    if(dynamic_dispatch.first) {
        return dynamic_dispatch.second;
    }

    if(returnsStruct) {
        if(!returnedStruct) {
            returnedValue = gen.builder->CreateAlloca(func_type->returnType->llvm_type(gen), nullptr);
        }
        args.emplace_back(returnedValue);
    }

    auto fn = decl != nullptr ? decl->llvm_func() : nullptr;
    to_llvm_args(gen, this, func_type.get(), values, args, &chain, until,0, nullptr, destructibles);

    llvm::Value* call_value;

    if(linked() && linked()->as_struct_member() != nullptr) { // means I'm calling a pointer inside a struct

        // creating access chain to the last member as an identifier instead of function call
        auto parent_access = parent_chain(this, chain);

        call_value = gen.builder->CreateCall(linked()->llvm_func_type(gen), parent_access.llvm_value(gen, nullptr), args);

    } else {
        call_value = call_with_args(this, fn, gen, args, chain, until, destructibles);
    }

    return returnedValue ? returnedValue : call_value;

}

llvm::Value* FunctionCall::access_chain_value(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &chain, unsigned until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles, BaseType* expected_type) {
    std::vector<llvm::Value *> args;
    auto value = llvm_chain_value(gen, args, chain, until, destructibles);
    call_move_fns_on_moved(gen, args);
    return value;
}

llvm::Value* FunctionCall::chain_value_with_callee(
        Codegen& gen,
        std::vector<std::unique_ptr<ChainValue>>& chain,
        unsigned int index,
        llvm::Value* grandpa_value,
        llvm::Value* callee_value,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {

    auto func_type = get_function_type();
    std::vector<llvm::Value*> args;

    llvm::Value* call_value;
    auto returnedValue = struct_return_in_args(gen, args, func_type.get(), nullptr);
    to_llvm_args(gen, this, func_type.get(), values, args, &chain, index, 0, grandpa_value, destructibles);

    if(linked() && linked()->as_struct_member()) {

        // creating access chain to the last member as an identifier instead of function call
        auto parent_access = parent_chain(this, chain);

        call_value = gen.builder->CreateCall(linked()->llvm_func_type(gen), parent_access.llvm_value(gen, nullptr), args);

    } else {

        call_value = call_with_callee(this, gen, args, chain, index, callee_value);

    }

    return returnedValue ? returnedValue : call_value;
}

llvm::InvokeInst *FunctionCall::llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind) {
    auto decl = linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    if(fn != nullptr) {
        auto type = decl->create_value_type();
        std::vector<llvm::Value *> args;
        std::vector<std::pair<Value*, llvm::Value*>> destructibles;
        to_llvm_args(gen, this, type->function_type(), values, args, nullptr, 0, 0, nullptr, destructibles);
        auto invoked = gen.builder->CreateInvoke(fn, normal, unwind, args);
        Value::destruct(gen, destructibles);
        return invoked;
    } else {
        gen.error("Unknown function call through invoke ", this);
        return nullptr;
    }
}

llvm::Value *FunctionCall::llvm_pointer(Codegen &gen) {
    throw std::runtime_error("llvm_pointer called on a function call");
}

bool FunctionCall::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return create_type()->linked_node()->add_child_index(gen, indexes, name);
}

void FunctionCall::call_move_fns_on_moved(Codegen &gen, std::vector<llvm::Value*>& args) {
    auto func_type = gen.current_func_type;
    unsigned i = 0;
    for(auto& value : values) {
        const auto chain = value->as_access_chain();
        if(chain && func_type->is_one_of_moved_chains(chain)) {
            auto known_t = chain->known_type();
            auto movable = known_t->get_direct_linked_movable_struct();
            const auto move_func = movable->move_func();
            const auto func = move_func->llvm_func();
            gen.builder->CreateCall(func, { args[i] });
        } else {
            const auto id = value->as_identifier();
            if(id && func_type->is_one_of_moved_id(id)) {
                const auto linked = id->linked;
                const auto linked_kind = linked->kind();
                if(linked_kind != ASTNodeKind::VarInitStmt) {

                }
            }
        }
        i++;
    }
}

llvm::AllocaInst *FunctionCall::access_chain_allocate(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &chain_values, unsigned int until, BaseType* expected_type) {
    auto func_type = get_function_type();
    if(func_type->returnType->value_type() == ValueType::Struct) {
        // we allocate the returned struct, llvm_chain_value function
        std::vector<llvm::Value *> args;
        std::vector<std::pair<Value*, llvm::Value*>> destructibles;
        auto alloc = (llvm::AllocaInst*) llvm_chain_value(gen, args, chain_values, until, destructibles);
        // call move functions on moved objects that were present in function call
        call_move_fns_on_moved(gen, args);
        // call destructors on destructible objects that were present in function call
        Value::destruct(gen, destructibles);
        return alloc;
    } else {
        return ChainValue::access_chain_allocate(gen, chain_values, until, expected_type);
    }
}

#endif

FunctionCall::FunctionCall(
        std::vector<std::unique_ptr<Value>> values,
        CSTToken* token
) : values(std::move(values)), token(token) {

}

uint64_t FunctionCall::byte_size(bool is64Bit) {
    return get_base_type()->byte_size(is64Bit);
}

void FunctionCall::link_values(SymbolResolver &linker) {
    auto& current_func = *linker.current_func_type;
    auto func_type = get_function_type();
    unsigned i = 0;
    while(i < values.size()) {
        auto& value = *values[i];
        value.link(linker, this, i);
        const auto expected_type = func_type->func_param_for_arg_at(i)->type.get();
        const auto expected_type_kind = expected_type->kind();
        if(expected_type_kind == BaseTypeKind::Reference) {
            i++;
            continue;
        }
        const auto expected_def = expected_type->get_ref_or_linked_struct(expected_type_kind);
        const auto type = value.known_type();
        const auto linked_def = type->get_direct_linked_struct();
        if(linked_def && linked_def->requires_moving()) {
            if(!expected_def) {
                if(expected_type_kind != BaseTypeKind::Any) {
                    linker.error("cannot move a struct to a non struct type", &value);
                }
                i++;
                continue;
            }
            if (expected_def == linked_def) {
                current_func.move_value(&value, linker);
            } else {
                const auto implicit = expected_def->implicit_constructor_for(&value);
                if(implicit) {
                    auto& param_type = *implicit->params[0]->type;
                    if(!param_type.is_reference()) { // not a reference type (requires moving)
                        current_func.move_value(&value, linker);
                    }
                } else {
                    linker.error("unknown value being moved, where the struct types don't match", &value);
                }
            }
        }
        i++;
    }
}

void FunctionCall::relink_values(SymbolResolver &linker) {
    unsigned i = 0;
    while(i < values.size()) {
        values[i]->relink_after_generic(linker, values[i], get_arg_type(i));
        i++;
    }
}

void FunctionCall::link_args_implicit_constructor(SymbolResolver &linker){
    auto func_type = get_function_type();
    unsigned i = 0;
    while(i < values.size()) {
        const auto param = func_type->func_param_for_arg_at(i);
        if (param) {
            auto implicit_constructor = param->type->implicit_constructor_for(values[i].get());
            if (implicit_constructor) {
                if(linker.preprocess) {
                    values[i] = call_with_arg(implicit_constructor, std::move(values[i]), linker);
                } else {
                    link_with_implicit_constructor(implicit_constructor, linker, values[i].get());
                }
            }
        }
        i++;
    }
}

void FunctionCall::link_gen_args(SymbolResolver &linker) {
    for(auto& type : generic_list) {
        type->link(linker, type);
    }
}

bool FunctionCall::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    link_gen_args(linker);
    link_values(linker);
    link_args_implicit_constructor(linker);
    return true;
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

BaseType* FunctionCall::get_arg_type(unsigned int index) {
    auto func_type = parent_val->known_type()->function_type();
    auto param = func_type->func_param_for_arg_at(index);
    return param->type.get();
}

int16_t FunctionCall::set_curr_itr_on_decl(FunctionDeclaration* decl) {
    int16_t prev_itr = -2;
    if(decl && !decl->generic_params.empty()) {
        prev_itr = decl->active_iteration;
        decl->set_active_iteration(generic_iteration);
    }
    return prev_itr;
}


void FunctionCall::infer_generic_args(ASTDiagnoser& diagnoser, std::vector<BaseType*>& inferred) {
    const auto func = safe_linked_func();
    if(!func) return;

    // going over function parameters to see which arguments have been given, if they do have a generic type
    // going over only explicit function params
    auto arg_offset = func->explicit_func_arg_offset();
    const auto values_size = values.size();
    while(arg_offset < values_size) {
        const auto param = func->params[arg_offset].get();
        const auto param_type = param->known_type();
        const auto arg_type = values[arg_offset]->known_type();
        if(!arg_type) {
#ifdef DEBUG
            std::cout << "couldn't get arg type " << values[arg_offset]->representation() + " in function call " + representation();
#endif
            arg_offset++;
            continue;
        }
        infer_types_by_args(diagnoser, func, generic_list.size(), param_type, arg_type, inferred, this);
        arg_offset++;
    }
}

void FunctionCall::infer_return_type(ASTDiagnoser& diagnoser, std::vector<BaseType*>& inferred, BaseType* expected_type) {
    const auto func = safe_linked_func();
    if(!func) return;
    infer_types_by_args(diagnoser, func, generic_list.size(), func->returnType.get(), expected_type, inferred, this);
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
                    diagnoser->error("couldn't find function that satisfies given arguments", this);
                }
            }
        }
    }
}

void FunctionCall::link_constructor(SymbolResolver &resolver) {
    // relinking parent with constructor of the struct
    // if it's linked with struct
    auto parent_id = parent_val->as_identifier();
    if(parent_id && parent_id->linked && parent_id->linked->as_struct_def()) {
        StructDefinition* parent_struct = parent_id->linked->as_struct_def();
        auto constructorFunc = parent_struct->constructor_func(values);
        if(constructorFunc) {
            parent_id->linked = constructorFunc;
            // calling a constructor of a generic struct where constructor is not generic
            if(constructorFunc->generic_params.empty() && parent_struct->is_generic()) {
                generic_iteration = parent_struct->register_generic_args(resolver, generic_list);
            }
        } else {
            resolver.error("struct with name " + parent_struct->name + " doesn't have a constructor that satisfies given arguments " + representation(), (AnnotableNode*) parent_struct);
        }
    }
}

void FunctionCall::relink_parent(ChainValue *parent) {
    parent_val = parent;
}

bool FunctionCall::find_link_in_parent(ChainValue* parent, SymbolResolver& resolver, BaseType* expected_type, bool link_implicit_constructor) {
    parent_val = parent;
    FunctionDeclaration* func_decl = safe_linked_func();
    int16_t prev_itr;
    relink_multi_func(&resolver);
    link_gen_args(resolver);
    link_constructor(resolver);
    link_values(resolver);
    if(func_decl && !func_decl->generic_params.empty()) {
        prev_itr = func_decl->active_iteration;
        generic_iteration = func_decl->register_call(resolver, this, expected_type);
        func_decl->set_active_iteration(generic_iteration);
    }
    relink_values(resolver);
    if(link_implicit_constructor) {
        link_args_implicit_constructor(resolver);
    }
    if(func_decl && !func_decl->generic_params.empty()) {
        func_decl->set_active_iteration(prev_itr);
    }
    return true;
}

bool FunctionCall::primitive() {
    return false;
}

bool FunctionCall::compile_time_computable() {
    auto func = safe_linked_func();
    return func && func->has_annotation(AnnotationKind::CompTime);
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

FunctionCall *FunctionCall::copy() {
    auto call = new FunctionCall({}, token);
    for(auto& value : values) {
        call->values.emplace_back(value->copy());
    }
    for(auto& gen_arg : generic_list) {
        call->generic_list.emplace_back(gen_arg->copy());
    }
    call->parent_val = parent_val;
    call->generic_iteration = generic_iteration;
    return call;
}

std::unique_ptr<BaseType> FunctionCall::create_type() {
    const auto func_decl = safe_linked_func();
    if(func_decl && func_decl->generic_params.empty() && func_decl->has_annotation(AnnotationKind::Constructor)) {
        const auto struct_def = func_decl->parent_node->as_struct_def();
        if(struct_def->is_generic()) {
            return std::make_unique<GenericType>(std::make_unique<LinkedType>(struct_def->name, struct_def, nullptr), generic_iteration);
        }
    }
    auto prev_itr = set_curr_itr_on_decl();
    auto func_type = create_function_type();
    auto pure_type = func_type->returnType->pure_type();
    if(prev_itr >= -1) safe_linked_func()->set_active_iteration(prev_itr);
    return std::unique_ptr<BaseType>(pure_type->copy());
}

std::unique_ptr<BaseType> FunctionCall::create_type(std::vector<std::unique_ptr<ChainValue>>& chain, unsigned int index) {
    auto data = get_grandpa_generic_struct(chain, index);
    if(data.first) {
        auto prev_itr = data.first->active_iteration;
        data.first->set_active_iteration(data.second);
        auto type = create_type();
        data.first->set_active_iteration(prev_itr);
        return type;
    } else {
        return create_type();
    }
}

hybrid_ptr<BaseType> FunctionCall::get_base_type() {
    const auto func_decl = safe_linked_func();
    if(func_decl && func_decl->generic_params.empty() && func_decl->has_annotation(AnnotationKind::Constructor)) {
        const auto struct_def = func_decl->parent_node->as_struct_def();
        if(struct_def->is_generic()) {
            return hybrid_ptr<BaseType> {new GenericType(std::make_unique<LinkedType>(struct_def->name, struct_def, nullptr), generic_iteration), true };
        }
    }
    auto prev_itr = set_curr_itr_on_decl();
    auto func_type = get_function_type();
    if(prev_itr >= -1) safe_linked_func()->set_active_iteration(prev_itr);
    if(func_type.get_will_free()) {
        return hybrid_ptr<BaseType> { func_type->returnType.release(), true };
    } else {
        return hybrid_ptr<BaseType> { func_type->returnType.get(), false };
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

BaseType* FunctionCall::known_type() {
    return parent_val->known_type()->function_type()->returnType.get();
}