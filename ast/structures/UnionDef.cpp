// Copyright (c) Qinetik 2024.

#include "UnnamedUnion.h"
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

llvm::Type *UnnamedUnion::llvm_type(Codegen &gen) {
    auto largest = largest_member();
    if(!largest) {
        gen.error("couldn't find the largest member in the union");
        return nullptr;
    }
    std::vector<llvm::Type*> members {largest->llvm_type(gen)};
    return llvm::StructType::get(*gen.ctx, members);
}

llvm::StructType* UnionDef::get_struct_type(Codegen &gen) {
    auto largest = largest_member();
    if(!largest) {
        gen.error("Couldn't determine the largest member of the union with name " + name);
        return nullptr;
    }
    if(!llvm_struct_type) {
        std::vector<llvm::Type*> members {largest->llvm_type(gen)};
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

UnnamedUnion::UnnamedUnion(std::string name) : BaseDefMember(std::move(name)) {

}

UnionDef::UnionDef(std::string name) : ExtendableMembersContainerNode(std::move(name)) {

}

void UnionDef::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void UnionDef::declare_and_link(SymbolResolver &linker) {
    MembersContainer::declare_and_link(linker);
}