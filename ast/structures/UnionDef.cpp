// Copyright (c) Qinetik 2024.

#include "UnionDef.h"
#include "FunctionDeclaration.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void UnionDef::code_gen(Codegen &gen) {
    for (auto &function: functions) {
        function.second->code_gen_union(gen, this);
    }
}

llvm::Type* UnionDef::largest_member_type(Codegen& gen) {
    StructMember* member = nullptr;
    for(auto& var : variables) {
        if(member == nullptr || var.second->byte_size(gen.is64Bit) > member->byte_size(gen.is64Bit)) {
            member = var.second.get();
        }
    }
    if(member) {
        return member->llvm_type(gen);
    } else {
        gen.error("Couldn't determine the largest member of the union with name " + name);
        return nullptr;
    }
}

llvm::StructType* UnionDef::get_struct_type(Codegen &gen) {
    if(!llvm_struct_type) {
        std::vector<llvm::Type*> members {largest_member_type(gen)};
        if(has_annotation(AnnotationKind::Anonymous)) {
            return llvm::StructType::get(*gen.ctx, members);
        }
        llvm_struct_type = llvm::StructType::create(*gen.ctx, members, "union." + name);
    }
    return llvm_struct_type;
}

llvm::Type *UnionDef::llvm_type(Codegen &gen) {
    return get_struct_type(gen);
}

#endif

UnionDef::UnionDef(std::string name) : name(std::move(name)) {

}

void UnionDef::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void UnionDef::declare_and_link(SymbolResolver &linker) {
    MembersContainer::declare_and_link(linker);
}