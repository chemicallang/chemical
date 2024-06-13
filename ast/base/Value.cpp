// Copyright (c) Qinetik 2024.

#include "Value.h"
#include "ast/values/StructValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/PointerType.h"
#include "ast/values/UIntValue.h"
#include "ast/values/AccessChain.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::AllocaInst* Value::llvm_allocate_with(Codegen& gen, const std::string& identifier, llvm::Value* value, llvm::Type* type) {
    auto x = gen.builder->CreateAlloca(type, nullptr, identifier);
    gen.builder->CreateStore(value, x);
    return x;
}

llvm::AllocaInst *Value::llvm_allocate(Codegen &gen, const std::string &identifier) {
    return llvm_allocate_with(gen, identifier, llvm_value(gen), llvm_type(gen));
}

llvm::GlobalVariable* Value::llvm_global_variable(Codegen& gen, bool is_const, const std::string& name) {
    return new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const, llvm::GlobalValue::LinkageTypes::PrivateLinkage, (llvm::Constant*) llvm_value(gen), name);
}

llvm::AllocaInst* Value::access_chain_allocate(Codegen& gen, const std::string& identifier, AccessChain* chain) {
    return llvm_allocate_with(gen, identifier, chain->llvm_value(gen), chain->llvm_type(gen));
}

unsigned int Value::store_in_struct(
        Codegen& gen,
        StructValue* parent,
        llvm::Value* allocated,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    auto elementPtr = gen.builder->CreateGEP(parent->llvm_type(gen), allocated, idxList, "", gen.inbounds);
    gen.builder->CreateStore(llvm_value(gen), elementPtr);
    return index + 1;
}

unsigned int Value::store_in_array(
        Codegen& gen,
        ArrayValue* parent,
        llvm::AllocaInst* ptr,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    auto elemPtr = gen.builder->CreateGEP(parent->llvm_type(gen), ptr, idxList, "", gen.inbounds);
    gen.builder->CreateStore(llvm_value(gen), elemPtr);
    return index + 1;
}

llvm::Value* Value::access_chain_value(Codegen &gen, std::vector<std::unique_ptr<Value>>& values, unsigned int until) {
    return gen.builder->CreateLoad(values[until - 1]->llvm_type(gen), access_chain_pointer(gen, values, until), "acc");
}

llvm::Value* Value::access_chain_pointer(Codegen &gen, std::vector<std::unique_ptr<Value>>& values, unsigned int until) {

    if(until == 1) {
        return values[0]->llvm_pointer(gen);
    }

    std::vector<llvm::Value*> idxList;

    // add member index of first value
    // if this is a index operator, only the integer index will be added since parent is nullptr
    if(!values[0]->add_member_index(gen, nullptr, idxList)) {
        gen.error("couldn't add member index for fragment '" + values[0]->representation() + "' in access chain '" + representation() + "'");
    }

    unsigned i = 1;
    while (i < until) {
        if(!values[i]->add_member_index(gen, values[i - 1].get(), idxList)) {
            gen.error("couldn't add member index for fragment '" + values[i]->representation() + "' in access chain '" + representation() + "'");
        }
        values[i]->find_link_in_parent(values[i - 1].get());
        i++;
    }
    if(values[0]->type_kind() == BaseTypeKind::Pointer) {
        auto ty = values[0]->create_type();
        return gen.builder->CreateGEP(((PointerType*) (ty.get()))->type->llvm_type(gen), values[0]->llvm_pointer(gen), idxList, "", gen.inbounds);
    } else {
        return gen.builder->CreateGEP(values[0]->llvm_type(gen), values[0]->llvm_pointer(gen), idxList, "", gen.inbounds);
    }
}

#endif

unsigned Value::as_uint() {
    return ((UIntValue*) this)->value;
}