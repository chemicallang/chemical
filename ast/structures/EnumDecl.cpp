// Copyright (c) Qinetik 2024.

#include "EnumDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/IntType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value *EnumMember::llvm_load(Codegen &gen) {
    return gen.builder->getInt32(index);
}

llvm::Type *EnumMember::llvm_type(Codegen &gen) {
    return gen.builder->getInt32Ty();
}

#endif

std::unique_ptr<BaseType> EnumMember::create_value_type() {
    return std::make_unique<IntType>();
}

ASTNode *EnumDeclaration::child(const std::string &name) {
    auto mem = members.find(name);
    if(mem == members.end()) {
        return nullptr;
    } else {
        return mem->second.get();
    }
}

void EnumDeclaration::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}