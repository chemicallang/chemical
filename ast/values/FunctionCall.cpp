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

AccessChain parent_chain(ASTAllocator& allocator, FunctionCall* call, std::vector<ChainValue*>& chain, int till) {
    AccessChain member_access(std::vector<ChainValue*> {}, nullptr, false, ZERO_LOC);
    unsigned i = 0;
    while(i < till) {
        if(chain[i] == call) {
            break;
        }
        member_access.values.emplace_back((ChainValue*)chain[i]->copy(allocator));
        i++;
    }
    return member_access;
}

AccessChain chain_range(ASTAllocator& allocator, FunctionCall* call, std::vector<ChainValue*>& chain, int till) {
    AccessChain member_access(std::vector<ChainValue*> {}, nullptr, false, ZERO_LOC);
    unsigned i = 0;
    while(i <= till) {
        member_access.values.emplace_back((ChainValue*) chain[i]->copy(allocator));
        i++;
    }
    return member_access;
}

AccessChain parent_chain(ASTAllocator& allocator, FunctionCall* call, std::vector<ChainValue*>& chain) {
    return parent_chain(allocator, call, chain, chain.size() - 1);
}

AccessChain grandparent_chain(ASTAllocator& allocator, FunctionCall* call, std::vector<ChainValue*>& chain) {
    return parent_chain(allocator, call, chain, chain.size() - 2);
}

void put_self_param(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<llvm::Value *>& args,
        std::vector<ChainValue*>* chain,
        unsigned int until,
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
#ifdef DEBUG
            if(!self_arg_val) {
                throw std::runtime_error("no self_arg_val passed to function call, however it takes a self arg");
            }
#endif
            args.emplace_back(self_arg_val);
        } else if(gen.current_func_type) {
            auto passing_self_arg = gen.current_func_type->get_self_param();
            if(passing_self_arg && passing_self_arg->type->is_same(self_param->type)) {
                args.emplace_back(passing_self_arg->llvm_load(gen));
            } else {
                gen.error("function without a self argument cannot call methods that require self arg", call);
                return;
            }
        }
    }
}

void put_implicit_params(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<llvm::Value *>& args,
        std::vector<ChainValue*>* chain,
        unsigned int until,
        llvm::Value* self_arg_val,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    for(auto& param : func_type->params) {
        if(param->is_implicit) {
            if(param->name == "self") {
                put_self_param(gen, call, func_type, args, chain, until, self_arg_val, destructibles);
            } else if(param->name == "other") {
                gen.error("unknown other implicit parameter", call);
            } else {
                auto found = gen.implicit_args.find(param->name);
                if(found != gen.implicit_args.end()) {
                    args.emplace_back(found->second);
                } else {
                    const auto between_param = gen.current_func_type->implicit_param_for(param->name);
                    if(between_param) {
                        args.emplace_back(gen.current_function->getArg(between_param->calculate_c_or_llvm_index()));
                    } else {
                        gen.error("couldn't provide implicit argument '" + param->name + "'", call);
                    }
                }
            }
        } else {
            break;
        }
    }
}

llvm::Value* arg_value(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        FunctionParam* func_param,
        Value* value_ptr,
        int i
) {
    const auto param_type = func_param->type;
    const auto param_type_kind = param_type->kind();

    auto implicit_constructor = param_type->implicit_constructor_for(gen.allocator, value_ptr);
    if (implicit_constructor) {
        value_ptr = call_with_arg(implicit_constructor, value_ptr, gen.allocator);
    }
    llvm::Value* argValue = nullptr;

    const auto value = value_ptr;
    const auto value_kind = value->val_kind();
    const auto is_val_stored_ptr = value->is_stored_ptr_or_ref(gen.allocator);

    const auto linked = param_type->get_direct_linked_node();

    const auto is_param_ref = param_type->is_reference(param_type_kind);

    if(
        (is_param_ref && !is_val_stored_ptr) || (
            linked && ASTNode::isStoredStructDecl(linked->kind()) &&
            (value->reference() && value->value_type() == ValueType::Struct) && !(value_kind == ValueKind::StructValue || value_kind == ValueKind::ArrayValue || value_kind == ValueKind::VariantCall)
    )) {
        argValue = value->llvm_pointer(gen);
    } else {
        if(i != -1) {
            argValue = value->llvm_arg_value(gen, param_type);
        } else {
            argValue = value->llvm_value(gen, param_type);
        }
    }
    if (func_type->isVariadic() && func_type->isInVarArgs(i) && argValue->getType()->isFloatTy()) {
        // Ensure proper type promotion for float values passed to printf
        argValue = gen.builder->CreateFPExt(argValue, llvm::Type::getDoubleTy(*gen.ctx));
    } else {
        if(value->is_ref_value()) {
            if (value->is_ref_moved()) {
                // move moved value using memcpy
                argValue = gen.move_by_allocate(func_param->type, value, argValue);
            } else if(gen.requires_memcpy_ref_struct(func_param->type, value)) {
                // non movable struct being passed directly, we should memcpy it
                auto type = value->llvm_type(gen);
                auto copy = gen.builder->CreateAlloca(type);
                gen.memcpy_struct(type, copy, argValue);
                argValue = copy;
            }
        }
        // pack it into a fat pointer, if the function expects a dynamic type
        argValue = gen.pack_dyn_obj(value, func_param->type, argValue);
    }
    return argValue;
}

void to_llvm_args(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<Value*>& values,
        std::vector<llvm::Value *>& args,
        std::vector<ChainValue*>* chain,
        unsigned int until,
        unsigned int start
) {

    unsigned i = start;

    for (i = start; i < values.size(); ++i) {

        const auto func_param = func_type->func_param_for_arg_at(i);

        args.emplace_back(arg_value(gen, call, func_type, func_param, values[i], (int) i));

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

    i += func_type->explicit_func_arg_offset();
    const auto func_param_size = func_type->params.size();
    while(i < func_param_size) {
        auto param = func_type->func_param_for_arg_at(i);
        if(param) {
            if(param->defValue) {
                args.emplace_back(arg_value(gen, call, func_type, param, param->defValue, -1));
            } else if(!func_type->isInVarArgs(i)) {
                gen.error("function param '" + param->name + "' doesn't have a default value, however no argument exists for it", call);
            }
        } else {
#ifdef DEBUG
            throw std::runtime_error("couldn't get function param");
#endif
        }
        i++;
    }

}

void to_llvm_args(
        Codegen& gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<Value*>& values,
        std::vector<llvm::Value *>& args,
        std::vector<ChainValue*>* chain,
        unsigned int until,
        unsigned int start,
        llvm::Value* self_arg_val,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    put_implicit_params(gen, call, func_type, args, chain, until, self_arg_val, destructibles);
    to_llvm_args(gen, call, func_type, values, args, chain, until, start);
}

llvm::Type *FunctionCall::llvm_type(Codegen &gen) {
    auto decl = safe_linked_func();
    int16_t prev_itr = set_curr_itr_on_decl();
    const auto type = create_type(gen.allocator)->llvm_type(gen);
    decl->set_active_iteration_safely(prev_itr);
    return type;
}

llvm::Type *FunctionCall::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return create_type(gen.allocator)->llvm_chain_type(gen, values, index);
}

llvm::FunctionType *FunctionCall::llvm_func_type(Codegen &gen) {
    return linked_func()->returnType->llvm_func_type(gen);
}

std::pair<llvm::Value*, llvm::FunctionType*>* FunctionCall::llvm_generic_func_data(ASTAllocator& allocator, std::vector<ChainValue*> &chain_values, unsigned int index) {
    auto gen_str = get_grandpa_generic_struct(allocator, chain_values, index);
    if(gen_str.first) {
        return &gen_str.first->llvm_generic_func_data(linked_func(), gen_str.second, generic_iteration);
    }
    return nullptr;
}

llvm::FunctionType *FunctionCall::llvm_linked_func_type(Codegen& gen, std::vector<ChainValue*> &chain_values, unsigned int index) {
    const auto generic_data = llvm_generic_func_data(gen.allocator, chain_values, index);
    if(generic_data) {
        return generic_data->second;
    }
    const auto func_type = function_type(gen.allocator);
    return func_type->llvm_func_type(gen);
}

llvm::Value* call_with_callee(
        FunctionCall* call,
        Codegen &gen,
        std::vector<llvm::Value*>& args,
        std::vector<ChainValue*> &chain_values,
        unsigned int index,
        llvm::Value* callee
) {
    return gen.builder->CreateCall(call->llvm_linked_func_type(gen, chain_values, index), callee, args);
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
        std::vector<ChainValue*> &chain_values,
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

    auto func_type = function_type(gen.allocator);
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
    llvm::Value* returned_value = struct_return_in_args(gen, args, func_type, nullptr);
    to_llvm_args(gen, this, func_type, values, args, &chain_values, index, 0, self_ptr, destructibles);

    llvm::Value* call_value = call_with_callee(this, gen, args, chain_values, index, callee);
    return { true, returned_value ? returned_value : call_value };

}

llvm::Value *FunctionCall::llvm_linked_func_callee(
        Codegen& gen,
        std::vector<ChainValue*> &chain_values,
        unsigned int index
) {
    const auto generic_data = llvm_generic_func_data(gen.allocator, chain_values, index);
    if(generic_data) {
        return generic_data->first;
    }
    if(linked() != nullptr) {
        if(linked()->as_function() == nullptr) {
            return linked()->llvm_load(gen);
        } else {
            return linked()->llvm_pointer(gen);
        }
    } else {
        return nullptr;
    }
}

llvm::Value *call_capturing_lambda(
        Codegen &gen,
        FunctionCall* call,
        FunctionType* func_type,
        std::vector<ChainValue*>* chain,
        unsigned int until,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    llvm::Value* grandpa = nullptr;
    llvm::Value* value;
    if(until > 1) {
        auto parent_access = parent_chain(gen.allocator, call, *chain);
        value = parent_access.llvm_value(gen, nullptr, &grandpa);
    } else {
        value = call->parent_val->llvm_value(gen);
    };
    auto dataPtr = gen.builder->CreateStructGEP(gen.fat_pointer_type(), value, 1);
    auto data = gen.builder->CreateLoad(gen.builder->getPtrTy(), dataPtr);
    std::vector<llvm::Value *> args;
    // TODO self param is being put first, the problem is that user probably expects that arguments are loaded first
    //   functions that take a implicit self param, this is ok, because their first argument will be self and should be loaded
    //   however functions that don't take a self reference, should load arguments first and then the func callee
    put_self_param(gen, call, func_type, args, chain, until, grandpa, destructibles);
    args.emplace_back(data);
    to_llvm_args(gen, call, func_type, call->values, args, chain, until, 0);
    auto structType = gen.fat_pointer_type();
    auto lambdaPtr = gen.builder->CreateStructGEP(structType, value, 0);
    auto lambda = gen.builder->CreateLoad(gen.builder->getPtrTy(), lambdaPtr);
    return gen.builder->CreateCall(func_type->llvm_func_type(gen), lambda, args);
}

void FunctionCall::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    const auto func = safe_linked_func();
    if(func && func->is_comptime()) {
        auto eval = gen.evaluated_func_calls.find(this);
        if(eval != gen.evaluated_func_calls.end()) {
            eval->second->llvm_destruct(gen, allocaInst);
            return;
        } else {
            // should this be reported ?
//            gen.info("couldn't find evaluated value of the function to destruct");
        }
    }
    auto funcType = function_type(gen.allocator);
    auto linked = funcType->returnType->linked_node();
    if(linked) {
        linked->llvm_destruct(gen, allocaInst);
    }
}

/**
 * check if chain is loadable, before loading it
 */
int is_chain_loadable(std::vector<ChainValue*>& chain) {
    auto& value = chain[0];
    const auto linked = value->linked_node();
    switch(linked->kind()) {
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::InterfaceDecl:
        case ASTNodeKind::ImplDecl:
        case ASTNodeKind::NamespaceDecl:
            return false;
        default:
            return true;
    }
}

llvm::Value* FunctionCall::llvm_chain_value(
        Codegen &gen,
        std::vector<llvm::Value*>& args,
        std::vector<ChainValue*>& chain,
        unsigned int until,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        llvm::Value* returnedStruct,
        llvm::Value* callee_value,
        llvm::Value* grandparent
) {

    auto func_type = function_type(gen.allocator);
    if(func_type->isCapturing()) {
        return call_capturing_lambda(gen, this, func_type, &chain, until, destructibles);
    }

    auto decl = safe_linked_func();
    if(decl && !decl->generic_params.empty()) {
        decl->set_active_iteration(generic_iteration);
    }
    llvm::Value* returnedValue = returnedStruct;
    auto returnsStruct = func_type->returnType->value_type() == ValueType::Struct;

    if(decl && decl->is_comptime()) {
        auto val = gen.eval_comptime(this, decl);
        if(!val) {
            return nullptr;
        }
        auto as_struct = val->as_struct_value();
        if(as_struct) {
            if(!returnedStruct) {
                returnedValue = gen.builder->CreateAlloca(func_type->returnType->llvm_type(gen), nullptr);
            }
            as_struct->initialize_alloca((llvm::AllocaInst*) returnedValue, gen, nullptr);
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

    if(!callee_value) {
        if(linked() && linked()->as_struct_member() != nullptr) {
            // creating access chain to the last member as an identifier instead of function call
            auto parent_access = parent_chain(gen.allocator, this, chain);
            callee_value = parent_access.llvm_value(gen, nullptr, &grandparent);
        } else {
            callee_value = llvm_linked_func_callee(gen, chain, until);
            if(callee_value == nullptr) {
                auto parent_access = parent_chain(gen.allocator, this, chain);
                int parent_index = (int) until - 2;
                if (parent_index >= 0 && parent_index < chain.size()) {
                    callee_value = parent_access.llvm_value(gen, nullptr, &grandparent);
                } else {
                    callee_value = parent_access.llvm_value(gen, nullptr);
                }
                if(callee_value == nullptr) {
                    gen.error("Couldn't get callee value for the function call to " + representation(), this);
                    return nullptr;
                }
            } else {
                if(is_chain_loadable(chain)) {
                    int parent_index = (int) until - 2;
                    if (parent_index >= 0 && parent_index < chain.size()) {
                        grandparent = llvm_load_chain_until(gen, chain, parent_index, destructibles);
                    }
                }
#ifdef DEBUG
                else if(func_type->has_self_param()) {
                    throw std::runtime_error("chain is not loadable however function requires a self argument");
                }
#endif
            }
        }
    }

    if(decl && decl->is_copy_fn()) {
        auto node = decl->params.front()->type->linked_node();
        auto def = node ? node->as_members_container() : nullptr;
        if(!def) {
            gen.error("couldn't figure out struct for which copy function is for", this);
            return nullptr;
        }
        if(!grandparent) {
            gen.error("couldn't figure out struct on which copy function is being called", this);
            return nullptr;
        }
        auto data = def->llvm_func_data(decl);
        args.emplace_back(grandparent);
        gen.builder->CreateCall(data.second, data.first, args);
        return returnedValue;
    }

    auto fn = decl != nullptr ? decl->llvm_func() : nullptr;
    to_llvm_args(gen, this, func_type, values, args, &chain, until,0, grandparent, destructibles);

    const auto llvm_func_type = llvm_linked_func_type(gen, chain, until);
    auto call_value = gen.builder->CreateCall(llvm_func_type, callee_value, args);

    return returnedValue ? returnedValue : call_value;

}

llvm::Value* FunctionCall::access_chain_value(Codegen &gen, std::vector<ChainValue*> &chain, unsigned until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles, BaseType* expected_type) {
    std::vector<llvm::Value *> args;
    auto value = llvm_chain_value(gen, args, chain, until, destructibles);
    return value;
}

llvm::Value* FunctionCall::chain_value_with_callee(
        Codegen& gen,
        std::vector<ChainValue*>& chain,
        unsigned int index,
        llvm::Value* grandpa_value,
        llvm::Value* callee_value,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    std::vector<llvm::Value *> args;
    auto value = llvm_chain_value(gen, args, chain, index, destructibles, nullptr, callee_value, grandpa_value);
    return value;
}

llvm::InvokeInst *FunctionCall::llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind) {
    auto decl = linked_func();
    auto fn = decl != nullptr ? (decl->llvm_func()) : nullptr;
    if(fn != nullptr) {
        auto type = decl->create_value_type(gen.allocator);
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
    return create_type(gen.allocator)->linked_node()->add_child_index(gen, indexes, name);
}

llvm::AllocaInst *FunctionCall::access_chain_allocate(Codegen &gen, std::vector<ChainValue*> &chain_values, unsigned int until, BaseType* expected_type) {
    auto func_type = function_type(gen.allocator);
    if(func_type->returnType->value_type() == ValueType::Struct) {
        // we allocate the returned struct, llvm_chain_value function
        std::vector<llvm::Value *> args;
        std::vector<std::pair<Value*, llvm::Value*>> destructibles;
        auto alloc = (llvm::AllocaInst*) llvm_chain_value(gen, args, chain_values, until, destructibles);
        // call destructors on destructible objects that were present in function call
        Value::destruct(gen, destructibles);
        return alloc;
    } else {
        return ChainValue::access_chain_allocate(gen, chain_values, until, expected_type);
    }
}

llvm::Value* FunctionCall::access_chain_assign_value(
        Codegen &gen,
        std::vector<ChainValue*> &chain,
        unsigned int until,
        std::vector<std::pair<Value*, llvm::Value*>> &destructibles,
        llvm::Value* lhsPtr,
        Value *lhs,
        BaseType *expected_type
) {
    auto func = safe_linked_func();
    if(func && func->returnType->value_type() == ValueType::Struct) {
        // we allocate the returned struct, llvm_chain_value function
        std::vector<llvm::Value *> args;
        // TODO very dirty way of doing this, the function returns struct and that's why the pointer is being used to assign to it
        //    returns nullptr because AssignStatement will assign the value for you, if you send it back, (THIS IS VERY BAD)
        llvm_chain_value(gen, args, chain, until, destructibles, lhsPtr);
        return nullptr;
    } else {
        return access_chain_value(gen, chain, until, destructibles, expected_type);
    }
}

#endif

FunctionCall::FunctionCall(
        std::vector<Value*> values,
        SourceLocation location
) : values(std::move(values)), location(location) {

}

uint64_t FunctionCall::byte_size(bool is64Bit) {
    return known_type()->byte_size(is64Bit);
}

void FunctionCall::link_values(SymbolResolver &linker, std::vector<bool>& properly_linked) {
    auto& current_func = *linker.current_func_type;
    auto func_type = function_type(linker.allocator);
    if(func_type && !func_type->data.signature_resolved) return;
    unsigned i = 0;
    while(i < values.size()) {
        auto& value_ptr = values[i];
        auto& value = *value_ptr;
        const auto param = func_type ? func_type->func_param_for_arg_at(i) : nullptr;
        const auto expected_type = param ? param->type : nullptr;
        if(value.link(linker, value_ptr, expected_type)) {
            properly_linked[i] = true;
            current_func.mark_moved_value(linker.allocator, &value, expected_type, linker);
        } else {
            properly_linked[i] = false;
        }
        i++;
    }
}

void FunctionCall::relink_values(SymbolResolver &linker) {
    auto func_type = function_type(linker.allocator);
    unsigned i = 0;
    while(i < values.size()) {
        const auto param = func_type ? func_type->func_param_for_arg_at(i) : nullptr;
        const auto expected_type = param ? param->type : nullptr;
        values[i]->relink_after_generic(linker, expected_type);
        i++;
    }
}

void FunctionCall::link_args_implicit_constructor(SymbolResolver &linker, std::vector<bool>& properly_linked){
    auto func_type = function_type(linker.allocator);
    if(!func_type || !func_type->data.signature_resolved) return;
    unsigned i = 0;
    while(i < values.size()) {
        const auto param = func_type->func_param_for_arg_at(i);
        if (param && properly_linked[i]) {
            const auto value = values[i];
            auto implicit_constructor = param->type->implicit_constructor_for(linker.allocator, value);
            if (implicit_constructor) {
                link_with_implicit_constructor(implicit_constructor, linker, value);
            } else if(!param->type->satisfies(linker.allocator, value, false)) {
                linker.unsatisfied_type_err(value, param->type);
            }
        }
        i++;
    }
}

void FunctionCall::link_gen_args(SymbolResolver &linker) {
    for(auto& type : generic_list) {
        type->link(linker);
    }
}

bool FunctionCall::link(SymbolResolver &linker, Value*& value_ptr, BaseType* expected_type) {
    link_gen_args(linker);
    std::vector<bool> properly_linked(values.size());
    link_values(linker, properly_linked);
    link_args_implicit_constructor(linker, properly_linked);
    return true;
}

//std::unique_ptr<FunctionType> FunctionCall::create_function_type() {
//    auto func_type = parent_val->known_type();
//    return std::unique_ptr<FunctionType>((FunctionType*) func_type.release());
//}

FunctionType* FunctionCall::function_type(ASTAllocator& allocator) {
    if(!parent_val) return nullptr;
    auto func_type = parent_val->create_type(allocator)->function_type();
    const auto func_decl = safe_linked_func();
    if(func_decl && func_decl->generic_params.empty() && func_decl->is_constructor_fn() && func_decl->parent_node) {
        const auto struct_def = func_decl->parent_node->as_struct_def();
        if(struct_def->is_generic()) {
            func_type->returnType = new (allocator.allocate<GenericType>()) GenericType(new (allocator.allocate<LinkedType>()) LinkedType(struct_def->name(), struct_def, location), generic_iteration);
        }
    }
    return func_type;
}

FunctionType* FunctionCall::known_func_type() {
    auto decl = safe_linked_func();
    if(decl) {
        return decl->function_type();
    }
    auto func_type = parent_val->known_type();
    if(func_type->function_type()) {
        return (FunctionType*) func_type;
    } else {
        return nullptr;
    }
}

BaseType* FunctionCall::get_arg_type(unsigned int index) {
    auto func_type = parent_val->known_type()->function_type();
    auto param = func_type->func_param_for_arg_at(index);
    return param->type;
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
        const auto param = func->params[arg_offset];
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
    infer_types_by_args(diagnoser, func, generic_list.size(), func->returnType, expected_type, inferred, this);
}

ASTNode *FunctionCall::linked_node() {
    const auto known = known_type();
    return known ? known->linked_node() : nullptr;
}

void FunctionCall::relink_multi_func(ASTAllocator& allocator, ASTDiagnoser* diagnoser) {
    auto parent = parent_val->as_identifier();
    if(parent) {
        if(parent->linked) {
            auto multi = parent->linked->as_multi_func_node();
            if(multi) {
                auto func = multi->func_for_call(allocator, values);
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
        auto constructorFunc = parent_struct->constructor_func(resolver.allocator, values);
        if(constructorFunc) {
            parent_id->linked = constructorFunc;
            // calling a constructor of a generic struct where constructor is not generic
            if(constructorFunc->generic_params.empty() && parent_struct->is_generic()) {
                generic_iteration = parent_struct->register_generic_args(resolver, generic_list);
            }
        } else {
            resolver.error("struct with name " + parent_struct->name() + " doesn't have a constructor that satisfies given arguments " + representation(), parent_id);
        }
    }
}

void FunctionCall::relink_parent(ChainValue *parent) {
    parent_val = parent;
}

bool FunctionCall::find_link_in_parent(ChainValue* first_value, ChainValue* grandpa, ChainValue* parent, SymbolResolver& resolver, BaseType* expected_type, bool link_implicit_constructor) {
    parent_val = parent;
    FunctionDeclaration* func_decl = safe_linked_func();
    if(func_decl) {
        if(func_decl->is_unsafe() && resolver.safe_context) {
            resolver.error("unsafe function with name should be called in an unsafe block", this);
        }
        const auto self_param = func_decl->get_self_param();
        if(self_param) {
            if(grandpa) {
                if(self_param->type->is_mutable(BaseTypeKind::Reference)) {
                    if(!first_value->check_is_mutable(resolver.current_func_type, resolver, false)) {
                        resolver.error("call requires a mutable implicit self argument, however current self argument is not mutable", this);
                    }
                }
            } else {
                const auto arg_self = resolver.current_func_type->get_self_param();
                if(!arg_self) {
                    resolver.error("cannot call function without an implicit self arg which is not present", this);
                } else if(self_param->type->is_mutable(BaseTypeKind::Reference) && !arg_self->type->is_mutable(BaseTypeKind::Reference)) {
                    resolver.error("call requires a mutable implicit self argument, however current self argument is not mutable", this);
                }
            }
        }
    }
    int16_t prev_itr;
    relink_multi_func(resolver.allocator, &resolver);
    link_gen_args(resolver);
    // this contains which args linked successfully
    std::vector<bool> properly_linked_args(values.size());
    // link the values, based on which constructor is determined
    link_values(resolver, properly_linked_args);
    // find the constructor based on linked values
    link_constructor(resolver);
    if(func_decl && !func_decl->generic_params.empty()) {
        prev_itr = func_decl->active_iteration;
        generic_iteration = func_decl->register_call(resolver, this, expected_type);
        func_decl->set_active_iteration(generic_iteration);
    }
    // relink values, because now we know the function type, so we know expected type
    relink_values(resolver);
    if(link_implicit_constructor) {
        link_args_implicit_constructor(resolver, properly_linked_args);
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
    return func && func->is_comptime();
}

Value *FunctionCall::find_in(InterpretScope &scope, Value *parent) {
    auto id = parent->as_identifier();
    if(id != nullptr) {
        return parent->call_member(scope, id->value.str(), values);
    } else {
        scope.error("No identifier for function call", this);
        return nullptr;
    }
}

Value* interpret_value(FunctionCall* call, InterpretScope &scope, Value* parent) {
    auto func = call->safe_linked_func();
    if (func) {
        return func->call(&scope, scope.allocator, call, parent);
    } else {
        scope.error("(function call) calling a function that is not found or has no body", call);
    }
    return nullptr;
}

Value *FunctionCall::scope_value(InterpretScope &scope) {
    return interpret_value(this, scope, nullptr);
}

Value* FunctionCall::evaluated_value(InterpretScope &scope) {
    return interpret_value(this, scope, nullptr);
}

Value* FunctionCall::evaluated_chain_value(InterpretScope &scope, Value* parent) {
    return interpret_value(this, scope, parent);
}

void FunctionCall::evaluate_children(InterpretScope &scope) {
    evaluate_values(values, scope);
}

FunctionCall *FunctionCall::copy(ASTAllocator& allocator) {
    auto call = new (allocator.allocate<FunctionCall>()) FunctionCall({}, location);
    for(auto& value : values) {
        call->values.emplace_back(value->copy(allocator));
    }
    for(auto& gen_arg : generic_list) {
        call->generic_list.emplace_back(gen_arg->copy(allocator));
    }
    call->parent_val = parent_val;
    call->generic_iteration = generic_iteration;
    return call;
}

BaseType* FunctionCall::create_type(ASTAllocator& allocator) {
    if(!parent_val) return nullptr;
    const auto func_decl = safe_linked_func();
    if(func_decl && func_decl->generic_params.empty() && func_decl->is_constructor_fn() && func_decl->parent_node) {
        const auto struct_def = func_decl->parent_node->as_struct_def();
        if(struct_def->is_generic()) {
            return new (allocator.allocate<GenericType>()) GenericType(new (allocator.allocate<LinkedType>()) LinkedType(struct_def->name(), struct_def, location), generic_iteration);
        }
    }
    auto prev_itr = set_curr_itr_on_decl();
    auto func_type = function_type(allocator);
    if(!func_type) return nullptr;
    auto pure_type = func_type->returnType->pure_type();
    if(prev_itr >= -1) func_decl->set_active_iteration(prev_itr);
    return pure_type;
}

//std::unique_ptr<BaseType> FunctionCall::create_type(std::vector<ChainValue*>& chain, unsigned int index) {
//    auto data = get_grandpa_generic_struct(chain, index);
//    if(data.first) {
//        auto prev_itr = data.first->active_iteration;
//        data.first->set_active_iteration(data.second);
//        auto type = create_type();
//        data.first->set_active_iteration(prev_itr);
//        return type;
//    } else {
//        return create_type();
//    }
//}

//hybrid_ptr<BaseType> FunctionCall::get_base_type() {
//    const auto func_decl = safe_linked_func();
//    if(func_decl && func_decl->generic_params.empty() && func_decl->has_annotation(AnnotationKind::Constructor)) {
//        const auto struct_def = func_decl->parent_node->as_struct_def();
//        if(struct_def->is_generic()) {
//            return hybrid_ptr<BaseType> {new GenericType(std::make_unique<LinkedType>(struct_def->name, struct_def, nullptr), generic_iteration), true };
//        }
//    }
//    auto prev_itr = set_curr_itr_on_decl();
//    auto func_type = get_function_type();
//    if(prev_itr >= -1) safe_linked_func()->set_active_iteration(prev_itr);
//    if(func_type.get_will_free()) {
//        return hybrid_ptr<BaseType> { func_type->returnType.release(), true };
//    } else {
//        return hybrid_ptr<BaseType> { func_type->returnType.get(), false };
//    }
//}

BaseTypeKind FunctionCall::type_kind() const {
    return const_cast<FunctionCall*>(this)->known_type()->kind();
}

ValueType FunctionCall::value_type() const {
    return const_cast<FunctionCall*>(this)->known_type()->value_type();
}

void FunctionCall::interpret(InterpretScope &scope) {
    evaluated_value(scope);
}

BaseType* FunctionCall::known_type() {
    if(parent_val) {
        const auto parent_type = parent_val->known_type();
        if(parent_type) {
            const auto func_type = parent_type->function_type();
            if(func_type) {
                return func_type->returnType;
            }
        }
    }
    return nullptr;
}