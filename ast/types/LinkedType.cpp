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
    if(!linked) {
        return false;
    }
    const auto other_kind = other->kind();
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
                        return other_linked->as_variant_member_unsafe()->parent() == linked->as_variant_def_unsafe();
                    } else {
                        return false;
                    }
                }
            } else {
                break;
            }
        }
        case ASTNodeKind::TypealiasStmt: {
            return linked->as_typealias_unsafe()->actual_type->satisfies(other->canonical());
        }
        case ASTNodeKind::EnumDecl: {
            return linked == other->get_direct_linked_node();
        }
        case ASTNodeKind::GenericTypeParam :{
            const auto param = linked->as_generic_type_param_unsafe();
            if(param->at_least_type) {
                return param->at_least_type->satisfies(other);
            } else {
                const auto known = param->known_type();
                if(known) {
                    return known->satisfies(other);
                } else {
                    // it's like a c++ template generic, which can take any type
                    return true;
                }
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

bool LinkedType::satisfies(ASTAllocator &allocator, Value *value, bool assignment) {
    const auto valueType = value->create_type(allocator);
    if(!valueType) return false;
    if(value->isValueIntegerLiteral()) {
        const auto known = linked->known_type();
        if (known) {
            const auto canonical = known->canonical();
            if(canonical->kind() == BaseTypeKind::IntN) {
                return true;
            }
        }
    }
    return satisfies(valueType);
}

bool NamedLinkedType::link(SymbolResolver &linker, SourceLocation loc) {
    const auto found = linker.find(link_name);
    if(found) {
        linked = found;
    } else if(linked == nullptr) {
        linker.error(loc) << "unresolved symbol, couldn't find referenced type " << link_name;
        return false;
    }
    return true;
}