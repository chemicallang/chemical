// Copyright (c) Chemical Language Foundation 2025.

#include "LinkedType.h"
#include "ast/statements/Typealias.h"
#include "StructType.h"
#include "compiler/SymbolResolver.h"
#include "ast/statements/VarInit.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/FunctionParam.h"

uint64_t LinkedType::byte_size(bool is64Bit) {
    return linked->byte_size(is64Bit);
}

bool LinkedType::satisfies(BaseType *other) {
    const auto other_kind = other->kind();
    if(!linked) {
        return false;
    }
    if(other_kind == BaseTypeKind::Linked && linked == ((LinkedType*) other)->linked) {
        return true;
    }
    const auto linked_kind = linked->kind();
    switch(linked_kind) {
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::InterfaceDecl: {
            const auto other_linked = other->get_direct_linked_node();
            if(other_linked) {
                if (linked == other_linked) {
                    return true;
                } else {
                    const auto container = other_linked->get_members_container();
                    return container && container->extends_node(linked);
                }
            } else {
                break;
            }
        }
        case ASTNodeKind::VariantDecl: {
            const auto other_linked = other->get_direct_linked_node();
            if(other_linked) {
                if (linked == other_linked) {
                    return true;
                } else {
                    const auto other_kind = other_linked->kind();
                    if(other_kind == ASTNodeKind::VariantMember) {
                        return other_linked->as_variant_member_unsafe()->parent_node == linked->as_variant_def_unsafe();
                    } else {
                        return false;
                    }
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
        case ASTNodeKind::GenericTypeParam :{
            const auto param = linked->as_generic_type_param_unsafe();
            if(param->usage.empty() && !param->at_least_type) {
                return true;
            } else {
                break;
            }
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

bool LinkedType::link(SymbolResolver &linker) {
    linked = linker.find(type);
    if(!linked) {
        linker.error(this) << "unresolved symbol, couldn't find referenced type " << type;
        return false;
    }
    return true;
}

ASTNode *LinkedType::linked_node() {
    return linked;
}