// Copyright (c) Chemical Language Foundation 2025.

#include "Assignment.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/FunctionType.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionParam.h"

void AssignStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(lhs->link_assign(linker, lhs, nullptr)) {
        BaseType* lhsType = lhs->create_type(linker.allocator);
        if(value->link(linker, value, lhsType)) {
            switch(assOp){
                case Operation::Assignment:
                    if (!lhsType->satisfies(linker.allocator, value, true)) {
                        linker.unsatisfied_type_err(value, lhsType);
                    }
                    break;
                case Operation::Addition:
                case Operation::Subtraction:
                    if(lhsType->kind() == BaseTypeKind::Pointer) {
                        const auto rhsType = value->create_type(linker.allocator)->canonical();
                        if(rhsType->kind() != BaseTypeKind::IntN) {
                            linker.unsatisfied_type_err(value, lhsType);
                        }
                    } else if (!lhsType->satisfies(linker.allocator, value, true)) {
                        linker.unsatisfied_type_err(value, lhsType);
                    }
                    break;
                default:
                    break;
            }
        }
        auto id = lhs->as_identifier();
        if(id) {
            auto linked = id->linked_node();
            auto linked_kind = linked->kind();
            if(linked_kind == ASTNodeKind::VarInitStmt) {
                auto init = linked->as_var_init_unsafe();
                init->set_has_assignment();
            } else if(linked_kind == ASTNodeKind::FunctionParam) {
                auto param = linked->as_func_param_unsafe();
                param->set_has_assignment();
            }
        }
        if(!lhs->check_is_mutable(linker.allocator, true)) {
            linker.error("cannot assign to a non mutable value", lhs);
        }
        auto& func_type = *linker.current_func_type;
        func_type.mark_moved_value(linker.allocator, value, lhs->known_type(), linker, true);
        func_type.mark_un_moved_lhs_value(lhs, lhs->known_type());
    }
}