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

llvm::Value* VariantCall::llvm_value(Codegen &gen, BaseType *type) {
    const auto member = chain->linked_node()->as_variant_member();
    const auto largest_member = member->parent_node->largest_member();
    llvm::Type* def_type;
    if(largest_member == member) {
        def_type = member->parent_node->llvm_type(gen);
    } else {
        def_type = member->parent_node->llvm_type_with_member(gen, member);
    }
    const auto allocated = gen.builder->CreateAlloca(def_type);
    const auto member_index = member->parent_node->direct_child_index(member->name);
    if(member_index == -1) {
        gen.error("couldn't find member index for the variant member with name '" + member->name + "'");
        return nullptr;
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
        value->store_in_struct(gen, nullptr, data_ptr, struct_type, { gen.builder->getInt32(0) }, i, param->second->type.get());
        i++;
    }
    return allocated;
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