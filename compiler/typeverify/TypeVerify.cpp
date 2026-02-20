
#include "TypeVerify.h"
#include "TypeVerifyAPI.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/MembersContainer.h"

void unsatisfied_type_err(ASTDiagnoser& diagnoser, ASTAllocator& allocator, Value* value, BaseType* type) {
    const auto val_type = value->getType();
    if(val_type) {
        diagnoser.error(value) << "value with type '" << val_type->representation() << "' does not satisfy type '" << type->representation() << "'";
    } else {
        diagnoser.error(value) << "value does not satisfy type '" << type->representation() << "'";
    }
}

void TypeVerifier::VisitArrayValue(ArrayValue* val) {
    RecursiveVisitor<TypeVerifier>::VisitArrayValue(val);
    auto& elemType = val->known_elem_type();
    for(const auto value : val->values) {
        if (!elemType->satisfies(value, false)) {
            unsatisfied_type_err(diagnoser, allocator, value, elemType);
        }
    }
}

void TypeVerifier::VisitFunctionCall(FunctionCall* call) {
    RecursiveVisitor<TypeVerifier>::VisitFunctionCall(call);
    auto func_type = call->function_type();
    if(!func_type || !func_type->data.signature_resolved) return;
    unsigned i = 0;
    while(i < call->values.size()) {
        const auto param = func_type->func_param_for_arg_at(i);
        if (param) {
            const auto value = call->values[i];
            auto implicit_constructor = param->type->implicit_constructor_for(value);
            if (implicit_constructor) {
                // TODO handle implicit constructor
            } else if(!param->type->satisfies(value, false)) {
                unsatisfied_type_err(diagnoser, allocator, value, param->type);
            }
        }
        i++;
    }
}

void TypeVerifier::VisitStructValue(StructValue* structValue) {
    RecursiveVisitor<TypeVerifier>::VisitStructValue(structValue);
    for (auto &val: structValue->values) {
        auto& val_ptr = val.second.value;
        const auto value = val_ptr;
        auto child_node = structValue->linked_member_or_struct_of(val.first);
        if(!child_node) {
            diagnoser.error(structValue) << "unresolved child '" << val.first << "' in struct declaration";
            continue;
        }
        const auto member = structValue->direct_variable(val.first);
        if(member) {
            const auto mem_type = member->known_type();
            auto implicit = mem_type->implicit_constructor_for(val_ptr);
            if(implicit) {
                // TODO handle implicit constructor
            } else if(!mem_type->satisfies(value, false)) {
                unsatisfied_type_err(diagnoser, allocator, value, mem_type);
            }
        }
    }
}

void verify_interface_implementation(ASTDiagnoser& diagnoser, MembersContainer* implementor, InterfaceDefinition* interface) {

    if(interface->is_extern() && interface->is_static()) {
        // compiler interfaces are extern and static
        return;
    }

    // First, verify inherited interfaces
    for (auto& inh : interface->inherited) {
        const auto canonical_node = inh.type->get_direct_linked_canonical_node();
        if(canonical_node->kind() == ASTNodeKind::InterfaceDecl) {
            verify_interface_implementation(diagnoser, implementor, canonical_node->as_interface_def_unsafe());
        }
    }

    // Iterate over all functions in the interface
    for (auto& node : interface->functions()) {
        if (node->kind() != ASTNodeKind::FunctionDecl) {
            continue;
        }
        const auto func_node = node->as_function_unsafe();
        if(func_node->body.has_value()) {
            // default implementation available
            continue;
        }
        const auto found = implementor->direct_child_function(func_node->name_view());
        if(found) {
            if(!found->is_override()) {
                diagnoser.error(found) << "expected function to have @override annotation to override the base function";
            }
        } else {
            diagnoser.error(implementor) << "type does not implement interface member '" << func_node->name_view() << "'";
        }
    }
}

void type_verify(ASTDiagnoser& diagnoser, ASTAllocator& allocator, std::span<ASTNode*> nodes) {
    TypeVerifier verifier(allocator, diagnoser);
    for(const auto node : nodes) {
        switch(node->kind()) {
            case ASTNodeKind::VarInitStmt: {
                const auto stmt = node->as_var_init_unsafe();
                if(!stmt->attrs.signature_resolved) {
                    continue;
                }
                auto& type = stmt->type;
                auto& value = stmt->value;
                if(type && value && !stmt->type->satisfies(value, false)) {
                    unsatisfied_type_err(diagnoser, allocator, value, type);
                }
                if(value) {
                    verifier.visit(stmt->value);
                }
                // check var init is non-destructible type
                if(stmt->is_never_destructed() == false && stmt->known_type()->get_destructor() != nullptr) {
                    diagnoser.error(stmt) << "top level variables or constants must be non-destructible, or must use @never_destructed annotation";
                }
                break;
            }
            case ASTNodeKind::NamespaceDecl: {
                const auto ns = node->as_namespace_unsafe();
                type_verify(diagnoser, allocator, ns->nodes);
                break;
            }
            case ASTNodeKind::StructDecl: {
                const auto structDecl = node->as_struct_def_unsafe();
                if(structDecl->is_abstract()) {
                    // TODO: check abstract structs
                    continue;
                }
                for (auto& inherited : structDecl->inherited) {
                    const auto interface_node = inherited.type->get_direct_linked_canonical_node();
                    if(interface_node->kind() == ASTNodeKind::InterfaceDecl) {
                        const auto interface = interface_node->as_interface_def_unsafe();
                        verify_interface_implementation(diagnoser, structDecl, interface);
                    }
                }
                break;
            }
            case ASTNodeKind::ImplDecl: {
                const auto implDecl = node->as_impl_def_unsafe();
                if (implDecl->interface_type) {
                     const auto interface_node = implDecl->interface_type->get_direct_linked_canonical_node();
                     if (interface_node->kind() == ASTNodeKind::InterfaceDecl) {
                         const auto interface = interface_node->as_interface_def_unsafe();
                         verify_interface_implementation(diagnoser, implDecl, interface);
                     }
                }
                break;
            }
            default:
                continue;
        }
    }
}