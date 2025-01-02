// Copyright (c) Qinetik 2024.

#include "EnumDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/IntType.h"
#include "ast/types/LinkedType.h"

BaseType* EnumMember::create_value_type(ASTAllocator& allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(parent_node->name_view(), (ASTNode*) parent_node, location);
}

//hybrid_ptr<BaseType> EnumMember::get_value_type() {
//    return hybrid_ptr<BaseType> { (BaseType*) &IntType::instance, false };
//}

BaseType* EnumMember::known_type() {
    return parent_node->known_type();
}

ASTNode *EnumDeclaration::child(const chem::string_view &name) {
    auto mem = members.find(name);
    if(mem == members.end()) {
        return nullptr;
    } else {
        return mem->second;
    }
}

void EnumDeclaration::declare_top_level(SymbolResolver &linker) {
    underlying_type->link(linker);
    linker.declare_node(name_view(), (ASTNode*) this, specifier(), false);
}

BaseType* EnumDeclaration::create_value_type(ASTAllocator& allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name_view(), (ASTNode*) this, location);
}

//hybrid_ptr<BaseType> EnumDeclaration::get_value_type() {
//    return hybrid_ptr<BaseType> { &type, false };
//}

BaseType* EnumDeclaration::known_type() {
    return &linked_type;
}
