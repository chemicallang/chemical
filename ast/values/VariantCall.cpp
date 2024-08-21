// Copyright (c) Qinetik 2024.

#include "VariantCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

bool VariantCall::initialize_allocated(Codegen &gen, llvm::Value* allocated, llvm::Type* def_type, VariantMember* member) {
    const auto member_index = member->parent_node->direct_child_index(member->name);
    if(member_index == -1) {
        gen.error("couldn't find member index for the variant member with name '" + member->name + "'");
        return false;
    }
    // storing the type index in the enum inside variant
    auto type_ptr = gen.builder->CreateGEP(def_type, allocated, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    gen.builder->CreateStore(gen.builder->getInt32(member_index), type_ptr);
    // storing the values of the variant inside it's struct
    auto data_ptr = gen.builder->CreateGEP(def_type, allocated, { gen.builder->getInt32(0), gen.builder->getInt32(1) }, "", gen.inbounds);
    const auto struct_type = member->llvm_type(gen);
    unsigned i = 0;
    for(auto& value : values) {
        const auto param = member->values.begin() + i;
        value->store_in_struct(gen, this, data_ptr, struct_type, { gen.builder->getInt32(0) }, i, param->second->type.get());
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

unsigned int VariantCall::store_in_array(Codegen &gen, ArrayValue *parent, llvm::AllocaInst *ptr, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) {
    const auto got = Value::get_element_pointer(gen, ((Value*) parent)->llvm_type(gen), ptr, idxList, index);
    initialize_allocated(gen, got);
    return index + 1;
}

#endif

VariantCall::VariantCall(std::unique_ptr<AccessChain> _chain) : chain(std::move(_chain)) {
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

void VariantCall::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    // we've already linked chain, when variant call is created, access chain is checked, so no need to link
    for(auto& type : generic_list) {
        type->link(linker, type);
    }
    for(auto& value : values) {
        value->link(linker, value);
    }
}

std::unique_ptr<BaseType> VariantCall::create_type() {
    const auto member = chain->linked_node()->as_variant_member();
    const auto largest_member = member->parent_node->largest_member();
    if(largest_member == member) {
        return member->parent_node->create_value_type();
    } else {
        // TODO when it's not the largest member, we must create the type so that
        //  it reflects that, so user can't assign other members that are smaller than this member
        return member->parent_node->create_value_type();
    }
}

hybrid_ptr<BaseType> VariantCall::get_base_type() {
    return hybrid_ptr<BaseType> { create_type().release(), true };
}

BaseType* VariantCall::known_type() {
    return nullptr;
}