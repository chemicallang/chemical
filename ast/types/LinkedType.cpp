// Copyright (c) Qinetik 2024.

#include "LinkedType.h"
#include "ast/statements/Typealias.h"
#include "StructType.h"
#include "compiler/SymbolResolver.h"
#include "ast/statements/VarInit.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/FunctionParam.h"

uint64_t LinkedType::byte_size(bool is64Bit) {
    return linked->byte_size(is64Bit);
}

ValueType LinkedType::value_type() const {
    return linked->value_type();
}

BaseType* LinkedType::pure_type() {
    if(linked) {
        const auto known = linked->known_type();
        return known ? known : this;
    } else {
        return this;
    }
}

bool LinkedType::satisfies(ValueType value_type) {
    if(linked->as_typealias() != nullptr) {
        return ((TypealiasStatement*) linked)->actual_type->satisfies(value_type);
    } else {
        return linked->value_type() == value_type;
    };
}

bool LinkedType::satisfies(BaseType *other) {
    const auto other_kind = other->kind();
    if(other_kind == BaseTypeKind::Linked) {
        return linked == ((LinkedType*) other)->linked;
    }
    const auto linked_kind = linked->kind();
    switch(linked_kind) {
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::InterfaceDecl:
        case ASTNodeKind::VariantDecl: {
            const auto other_linked = other->get_direct_linked_node(other_kind);
            if(other_linked) {
                if (linked == other_linked) {
                    return true;
                } else {
                    const auto container = other_linked->get_members_container(other_linked->kind());
                    return container && container->extends_node(linked);
                }
            } else {
                break;
            }
        }
        case ASTNodeKind::TypealiasStmt: {
            return linked->as_typealias_unsafe()->actual_type->satisfies(other->pure_type());
        }
        case ASTNodeKind::EnumDecl: {
            return linked == other->get_direct_linked_node();
        }
        default:
            break;
    }
    auto known = linked->known_type();
    if(known && known != this) {
        return known->satisfies(other);
    }
    return false;
}

void LinkedType::link(SymbolResolver &linker) {
    linked = linker.find(type);
    if(!linked) {
        linker.error("unresolved symbol, couldn't find referenced type " + type, this);
    }
}

ASTNode *LinkedType::linked_node() {
    return linked;
}