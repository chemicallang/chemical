// Copyright (c) Chemical Language Foundation 2025.

#include "EnumDeclaration.h"
#include "ast/types/LinkedType.h"
#include "ast/types/IntNType.h"

BaseType* EnumMember::known_type() {
    return parent()->known_type();
};

Value* EnumMember::evaluate(ASTAllocator& allocator, TypeBuilder& typeBuilder, SourceLocation location) {
    // we aren't supplying debug information for the load of this value
    // it's a constant integer, but we need to supply this location information somehow
    if(init_value) {
        return init_value;
    } else {
        return parent()->get_underlying_integer_type()->create(allocator, typeBuilder, get_default_index(), location);
    }
}

EnumDeclaration* EnumDeclaration::get_inherited_enum_decl() {
    const auto inherited = underlying_type->get_direct_linked_node();
    return inherited && inherited->kind() == ASTNodeKind::EnumDecl ? inherited->as_enum_decl_unsafe() : nullptr;
}