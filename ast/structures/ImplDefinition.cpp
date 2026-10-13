// Copyright (c) Chemical Language Foundation 2025.

#include "ImplDefinition.h"

#include "GenericInterfaceDecl.h"
#include "StructMember.h"
#include "ast/structures/StructDefinition.h"
#include "compiler/frontend/AnnotationController.h"
#include "std/except.h"


uint64_t ImplDefinition::byte_size(const TargetData& target) {
#ifdef DEBUG
    CHEM_THROW_RUNTIME("ImplDefinition::byte_size byte_size on impl definition");
#endif
    return 0;
}

void InterfaceDefinition::register_impl(ImplDefinition* definition) {
    attrs.has_implementation = true;
    if (definition->struct_type == nullptr) return;
    const auto linked = definition->struct_type->get_direct_linked_canonical_node();
    if (linked) {
        switch (linked->kind()) {
            case ASTNodeKind::StructDecl:
            case ASTNodeKind::UnionDecl:
            case ASTNodeKind::VariantDecl:
                register_use(linked->as_extendable_members_container_unsafe());
                register_use_to_inherited_interfaces(linked->as_extendable_members_container_unsafe());
                return;
            default:
                return;
        }
    }
}