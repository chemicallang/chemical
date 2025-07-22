// Copyright (c) Chemical Language Foundation 2025.

#include "EnumDeclaration.h"
#include "ast/types/IntType.h"
#include "ast/types/LinkedType.h"

BaseType* EnumMember::known_type() {
    return parent()->known_type();
};

EnumDeclaration* EnumDeclaration::get_inherited_enum_decl() {
    const auto inherited = underlying_type->get_direct_linked_node();
    return inherited && inherited->kind() == ASTNodeKind::EnumDecl ? inherited->as_enum_decl_unsafe() : nullptr;
}

BaseType* EnumDeclaration::known_type() {
    return &linked_type;
}
