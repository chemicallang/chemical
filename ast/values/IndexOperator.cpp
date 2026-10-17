// Copyright (c) Chemical Language Foundation 2025.

#include "IndexOperator.h"
#include "ast/values/AccessChain.h"
#include "ast/base/ASTNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/VoidType.h"
#include "ast/statements/Typealias.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/base/InterpretScope.h"
#include "PointerValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/lab/LabBuildCompiler.h"
#include <cstring>


// TODO: make this function on BaseType
BaseType* get_child_type(const TypeBuilder& typeBuilder, BaseType* type, bool unwrap_ref = false) {
    switch(type->kind()) {
        case BaseTypeKind::Array:
            return type->as_array_type_unsafe()->elem_type;
        case BaseTypeKind::Pointer:
            return type->as_pointer_type_unsafe()->type;
        case BaseTypeKind::Reference:
            // since reference is automatically dereferenced
            if(unwrap_ref) {
                return get_child_type(typeBuilder, type->as_reference_type_unsafe()->type, false);
            } else {
                return type->as_reference_type_unsafe()->type;
            }
        case BaseTypeKind::String:
            return typeBuilder.getCharType();
        case BaseTypeKind::Linked:{
            const auto linked = type->as_linked_type_unsafe()->linked;
            if(linked->kind() == ASTNodeKind::TypealiasStmt) {
                return get_child_type(typeBuilder, linked->as_typealias_unsafe()->actual_type, unwrap_ref);
            } else {
                return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

void IndexOperator::determine_type(const TypeBuilder& typeBuilder, const CoreNodes& coreNodes, const ImplementationsIndex& implsIndex, ASTDiagnoser& diagnoser) {
    auto current_type = parent_val->getType();
    const auto can_node = current_type->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            const auto func = implsIndex.get_index_op_impl(coreNodes, container);
            if (func == nullptr) {
                diagnoser.error(this) << "couldn't find 'index' or 'index_mut' function implementation for operator overloading";
                setType(typeBuilder.getVoidType());
                return;
            }
            // a single function with name 'index' is present
            if (func->params.size() != 2) {
                diagnoser.error(this) << "expected 'index' operator function to have exactly two parameters";
                setType(typeBuilder.getVoidType());
                return;
            }
            const auto childType = get_child_type(typeBuilder, func->returnType);
            if(childType) {
                setType(childType);
            } else {
                setType(typeBuilder.getVoidType());
            }
            return;
        }
    }
    const auto childType = get_child_type(typeBuilder, current_type, true);
    if(childType) {
        setType(childType);
    } else {
        setType(typeBuilder.getVoidType());
    }
}

ASTNode *IndexOperator::linked_node() {
    const auto value_type = getType();
    return value_type ? value_type->linked_node() : nullptr;
}
