// Copyright (c) Chemical Language Foundation 2025.

#include "DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"

bool DynamicType::satisfies(BaseType *type) {
    const auto type_kind = type->kind();
    if(type_kind == BaseTypeKind::Dynamic) {
        return referenced->satisfies(((DynamicType*) type)->referenced);
    } else {
        const auto interface = referenced->get_direct_linked_interface();
        if(interface) {
            const auto linked = type->get_direct_linked_struct();
            if(linked) {
                auto found = interface->users.find(linked);
                return found != interface->users.end();
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
}