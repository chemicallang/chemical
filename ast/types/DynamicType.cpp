// Copyright (c) Qinetik 2024.

#include "DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"

DynamicType::DynamicType(BaseType* referenced, SourceLocation location) : referenced(referenced), TokenizedBaseType(location) {

}

bool DynamicType::link(SymbolResolver &linker) {
    return referenced->link(linker);
}

bool DynamicType::satisfies(BaseType *type) {
    const auto type_kind = type->kind();
    if(type_kind == BaseTypeKind::Dynamic) {
        return referenced->satisfies(((DynamicType*) type)->referenced);
    } else {
        const auto interface = referenced->get_direct_linked_interface(referenced->kind());
        if(interface) {
            const auto linked = type->get_direct_linked_struct(type_kind);
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