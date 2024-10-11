// Copyright (c) Qinetik 2024.

#include "VariantCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/base/BaseType.h"
#include "ast/types/GenericType.h"
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
    auto itr = member->values.begin();
    for(auto& value_ptr : values) {

        const auto param = itr->second;
        const auto param_type = param->type;

        auto implicit_constructor = param_type->implicit_constructor_for(gen.allocator, value_ptr);
        if (implicit_constructor) {
            // replace calls to implicit constructor with actual calls
            value_ptr = call_with_arg(implicit_constructor, value_ptr, gen.allocator);
        } else {
            if(param_type->kind() == BaseTypeKind::Reference) {
                // store reference when it's a implicit reference
                std::vector<llvm::Value*> idxList { gen.builder->getInt32(0) };
                auto elementPtr = Value::get_element_pointer(gen, struct_type, data_ptr, idxList, i);
                const auto val = value_ptr->llvm_pointer(gen);
                gen.builder->CreateStore(val, elementPtr);
                continue;
            }
        }

        auto& value = *value_ptr;
        bool moved = false;
        if(value_ptr->is_ref_moved()) {
            // since it will be moved, we will std memcpy it into current pointer
            std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
            auto elementPtr = Value::get_element_pointer(gen, struct_type, data_ptr, idx, i);
            moved = gen.move_by_memcpy(param_type, value_ptr, elementPtr, value_ptr->llvm_value(gen));
        }
        if(!moved) {
            if(gen.requires_memcpy_ref_struct(param_type, value_ptr)) {
                std::vector<llvm::Value*> idxList { gen.builder->getInt32(0) };
                auto elementPtr = Value::get_element_pointer(gen, struct_type, data_ptr, idxList, i);
                gen.memcpy_struct(value_ptr->llvm_type(gen), elementPtr, value_ptr->llvm_value(gen, nullptr));
            } else {
                value.store_in_struct(gen, this, data_ptr, struct_type, {gen.builder->getInt32(0)}, i, param_type);
            }
        }
        itr++;
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

VariantCall::VariantCall(AccessChain* _chain, CSTToken* token) : chain(_chain), token(token) {
    const auto func_call = chain->values.back()->as_func_call();
    if(func_call) {
        for(auto& value : func_call->values) {
            values.emplace_back(value);
        }
        for(auto& type : func_call->generic_list) {
            generic_list.emplace_back(type);
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
        values[i]->relink_after_generic(linker, itr->second->type);
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
        const auto param_type = itr->second->type;
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
        const auto value = values[i];
        const auto type = itr->second->type;
        auto implicit_constructor = type->implicit_constructor_for(linker.allocator, value);
        if (implicit_constructor) {
            link_with_implicit_constructor(implicit_constructor, linker, value);
        } else if(!type->satisfies(linker.allocator, value)) {
            linker.unsatisfied_type_err(value, type);
        }
        i++;
        itr++;
    }
}

bool VariantCall::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    const auto member = chain->linked_node()->as_variant_member();
    auto& current_func = *linker.current_func_type;
    // we've already linked chain, when variant call is created, access chain is checked, so no need to link
    for(auto& type : generic_list) {
        type->link(linker);
    }
    unsigned i = 0;
    for(auto& mem_value_ptr : values) {
        auto& value = *mem_value_ptr;
        if(value.link(linker, mem_value_ptr)) {
            const auto param = member->values.begin() + i;
            current_func.mark_moved_value(linker.allocator, &value, param->second->type, linker, true);
        }
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
    set_created_type(*linker.ast_allocator);
    return true;
}

void VariantCall::set_created_type(ASTAllocator& allocator) {
    const auto member = chain->linked_node()->as_variant_member();
    const auto parent_node = member->parent_node;
    const auto largest_member = parent_node->largest_member();
    if(largest_member == member) {
        cached_type = parent_node->create_value_type(allocator);
    } else {
        // TODO when it's not the largest member, we must create the type so that
        //  it reflects that, so user can't assign other members that are smaller than this member
        cached_type = parent_node->create_value_type(allocator);
    }
    if(!parent_node->generic_params.empty()) {
        const auto gen_type = ((GenericType*) cached_type);
        gen_type->generic_iteration = generic_iteration;
    }
}

BaseType* VariantCall::create_type(ASTAllocator& allocator) {
    set_created_type(allocator);
    return cached_type;
}

BaseType* VariantCall::known_type() {
    return cached_type;
}