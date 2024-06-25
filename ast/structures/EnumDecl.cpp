// Copyright (c) Qinetik 2024.

#include "EnumDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/IntType.h"

std::unique_ptr<BaseType> EnumMember::create_value_type() {
    return std::make_unique<IntType>();
}

hybrid_ptr<BaseType> EnumMember::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
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