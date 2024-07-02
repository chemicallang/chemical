// Copyright (c) Qinetik 2024.

#include "UnnamedUnion.h"
#include "UnionDef.h"
#include "FunctionDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferencedType.h"

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

llvm::Type *UnionDef::llvm_type(Codegen &gen) {
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

#endif

UnnamedUnion::UnnamedUnion(std::string name, ASTNode* parent_node) : BaseDefMember(std::move(name)), parent_node(parent_node) {

}

hybrid_ptr<BaseType> UnnamedUnion::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

UnionDef::UnionDef(std::string name, ASTNode* parent_node) : ExtendableMembersContainerNode(std::move(name)), parent_node(parent_node) {

}

BaseType *UnionDef::copy() const {
    return new ReferencedType(name, (ASTNode*) this);
}

BaseType *UnnamedUnion::copy() const {
    return new ReferencedType(name, (ASTNode*) this);
}

std::unique_ptr<BaseType> UnionDef::create_value_type() {
    return std::unique_ptr<BaseType>(copy());
}

hybrid_ptr<BaseType> UnionDef::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

void UnionDef::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void UnionDef::declare_and_link(SymbolResolver &linker) {
    MembersContainer::declare_and_link(linker);
}

void UnnamedUnion::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    VariablesContainer::declare_and_link(linker);
    linker.scope_end();
    linker.declare(name, this);
}