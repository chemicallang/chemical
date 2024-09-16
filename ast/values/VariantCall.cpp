// Copyright (c) Qinetik 2024.

#include "VariantCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/utils/GenericUtils.h"
#include "ast/utils/ASTUtils.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

bool VariantCall::initialize_allocated(Codegen &gen, llvm::Value* allocated, llvm::Type* def_type, VariantMember* member) {
    const auto member_index = member->parent_node->direct_child_index(member->name);
    if(member_index == -1) {
        gen.error("couldn't find member index for the variant member with name '" + member->name + "'", this);
        return false;
    }
    // storing the type index in the enum inside variant
    auto type_ptr = gen.builder->CreateGEP(def_type, allocated, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    gen.builder->CreateStore(gen.builder->getInt32(member_index), type_ptr);
    // storing the values of the variant inside it's struct
    auto data_ptr = gen.builder->CreateGEP(def_type, allocated, { gen.builder->getInt32(0), gen.builder->getInt32(1) }, "", gen.inbounds);
    const auto struct_type = member->llvm_type(gen);
    unsigned i = 0;
    auto& current_func_type = *gen.current_func_type;
    for(auto& value_ptr : values) {
        auto& value = *value_ptr;
        const auto param = member->values.begin() + i;
        auto& param_type = *param->second->type;
        auto movable_value = current_func_type.movable_value(gen, value_ptr.get());
        if(movable_value == nullptr) {
            if(gen.requires_memcpy_ref_struct(&param_type, value_ptr.get())) {
                std::vector<llvm::Value*> idxList { gen.builder->getInt32(0) };
                auto elementPtr = Value::get_element_pointer(gen, struct_type, data_ptr, idxList, i);
                gen.memcpy_struct(value_ptr->llvm_type(gen), value_ptr.get(), elementPtr, value_ptr->llvm_value(gen, nullptr));
            } else {
                value.store_in_struct(gen, this, data_ptr, struct_type, {gen.builder->getInt32(0)}, i,
                                      &param_type);
            }
        } else {
            // since it will be moved, we will std memcpy it into current pointer
            std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
            auto elementPtr = Value::get_element_pointer(gen, struct_type, data_ptr, idx, i);
            current_func_type.move_by_memcpy(gen, &param_type, value_ptr.get(), elementPtr, movable_value);
        }
        i++;
    }
    return true;
}

llvm::Value* VariantCall::initialize_allocated(Codegen &gen, llvm::Value* allocated) {
    const auto member = chain->linked_node()->as_variant_member();
    auto def_type = llvm_type(gen);
    if(!allocated) {
        allocated = gen.builder->CreateAlloca(def_type);
    }
    if(initialize_allocated(gen, allocated, def_type, member)) {
        return allocated;
    }
    return nullptr;
}

llvm::Value* VariantCall::llvm_value(Codegen &gen, BaseType *type) {
    return initialize_allocated(gen, nullptr);
}

llvm::AllocaInst* VariantCall::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    const auto member = chain->linked_node()->as_variant_member();
    auto def_type = expected_type ? expected_type->llvm_type(gen) : llvm_type(gen);
    auto allocated = gen.builder->CreateAlloca(def_type);
    if(initialize_allocated(gen, allocated, def_type, member)) {
        return allocated;
    }
    return nullptr;
}

llvm::Type* VariantCall::llvm_type(Codegen &gen) {
    const auto member = chain->linked_node()->as_variant_member();
    const auto largest_member = member->parent_node->largest_member();
    llvm::Type* def_type;
    if(largest_member == member) {
        def_type = member->parent_node->llvm_type(gen);
    } else {
        def_type = member->parent_node->llvm_type_with_member(gen, member);
    }
    return def_type;
}

unsigned int VariantCall::store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) {
    const auto ptr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    initialize_allocated(gen, ptr);
    return index + 1;
}

unsigned int VariantCall::store_in_array(
        Codegen &gen,
        Value *parent,
        llvm::Value *allocated,
        llvm::Type *allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType *expected_type
) {
    const auto got = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    initialize_allocated(gen, got);
    return index + 1;
}

void VariantCall::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    const auto def = get_definition();
    def->llvm_destruct(gen, allocaInst);
}

#endif

VariantCall::VariantCall(std::unique_ptr<AccessChain> _chain, CSTToken* token) : chain(std::move(_chain)), token(token) {
    const auto func_call = chain->values.back()->as_func_call();
    if(func_call) {
        for(auto& value : func_call->values) {
            values.emplace_back(std::move(value));
        }
        for(auto& type : func_call->generic_list) {
            generic_list.emplace_back(std::move(type));
        }
        chain->values.pop_back();
    }
}

VariantMember* VariantCall::get_member() {
    return chain->linked_node()->as_variant_member();
}

VariantDefinition* VariantCall::get_definition() {
    return get_member()->parent_node;
}

void VariantCall::relink_values(SymbolResolver& linker) {
    const auto member = get_member();
    unsigned i = 0;
    auto itr = member->values.begin();
    while(i < values.size()) {
        values[i]->relink_after_generic(linker, values[i], itr->second->type.get());
        i++;
        itr++;
    }
}

void VariantCall::infer_generic_args(ASTDiagnoser& diagnoser, std::vector<BaseType*>& inferred) {
    const auto member = get_member();
    const auto def = member->parent_node;
    // going over function parameters to see which arguments have been given, if they do have a generic type
    // going over only explicit function params
    const auto values_size = values.size();
    auto arg_offset = 0;
    auto itr = member->values.begin();
    while(arg_offset < values_size) {
        const auto param_type = itr->second->type.get();
        const auto arg_type = values[arg_offset]->known_type();
        if(!arg_type) {
#ifdef DEBUG
            std::cout << "couldn't get arg type " << values[arg_offset]->representation() + " in function call " + representation();
#endif
            arg_offset++;
            continue;
        }
        infer_types_by_args(diagnoser, def, generic_list.size(), param_type, arg_type, inferred, this);
        arg_offset++;
        itr++;
    }
}

void VariantCall::link_args_implicit_constructor(SymbolResolver &linker) {
    const auto member = get_member();
    unsigned i = 0;
    auto itr = member->values.begin();
    while(i < values.size()) {
        auto implicit_constructor = itr->second->type->implicit_constructor_for(values[i].get());
        if (implicit_constructor) {
            if(linker.preprocess) {
                values[i] = call_with_arg(implicit_constructor, std::move(values[i]), linker);
            } else {
                link_with_implicit_constructor(implicit_constructor, linker, values[i].get());
            }
        }
        i++;
        itr++;
    }
}

bool VariantCall::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *expected_type) {
    const auto member = chain->linked_node()->as_variant_member();
    auto& current_func = *linker.current_func_type;
    // we've already linked chain, when variant call is created, access chain is checked, so no need to link
    for(auto& type : generic_list) {
        type->link(linker, type);
    }
    unsigned i = 0;
    for(auto& mem_value_ptr : values) {
        auto& value = *mem_value_ptr;
        value.link(linker, mem_value_ptr);
        const auto param = member->values.begin() + i;
        current_func.mark_moved_value(&value, param->second->type.get(), linker, true);
        i++;
    }
    int16_t prev_itr;
    const auto def = get_definition();
    if(!def->generic_params.empty()) {
        prev_itr = def->active_iteration;
        generic_iteration = def->register_call(linker, this, expected_type);
        def->set_active_iteration(generic_iteration);
    }
    relink_values(linker);
    link_args_implicit_constructor(linker);
    if(!def->generic_params.empty()) {
        def->set_active_iteration(prev_itr);
    }
    return true;
}

void VariantCall::set_created_type() {
    const auto member = chain->linked_node()->as_variant_member();
    const auto largest_member = member->parent_node->largest_member();
    if(largest_member == member) {
        cached_type = member->parent_node->create_value_type();
    } else {
        // TODO when it's not the largest member, we must create the type so that
        //  it reflects that, so user can't assign other members that are smaller than this member
        cached_type = member->parent_node->create_value_type();
    }
}

std::unique_ptr<BaseType> VariantCall::create_type() {
    if(!cached_type) {
        set_created_type();
    }
    return cached_type->copy_unique();
}

hybrid_ptr<BaseType> VariantCall::get_base_type() {
    if(!cached_type) {
        set_created_type();
    }
    return hybrid_ptr<BaseType> { cached_type.get(), false };
}

BaseType* VariantCall::known_type() {
    if(!cached_type) {
        set_created_type();
    }
    return cached_type.get();
}