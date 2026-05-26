
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
#include "compiler/symres/ImplementationsIndex.h"
#include "core/source/LocationManager.h"

void unsatisfied_type_err(ASTDiagnoser& diagnoser, Value* value, BaseType* type) {
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
    if (elemType) {
        if (elemType->kind() == BaseTypeKind::Void) {
            diagnoser.error("array element type cannot be void", val);
            return;
        }
        const auto def = elemType->get_direct_linked_struct();
        if(def) {
            unsigned i = 0;
            while (i < val->values.size()) {
                const auto value = val->values[i];
                visit(value);
                const auto implicit = def->implicit_constructor_func(value);
                if(implicit) {
                    // TODO: handle implicit constructor
                } else if(!elemType->satisfies(value, false)) {
                    unsatisfied_type_err(diagnoser, value, elemType);
                }
                i++;
            }
            return;
        }
        for(const auto value : val->values) {
            visit(value);
            if(!elemType->satisfies(value, false)) {
                unsatisfied_type_err(diagnoser, value, elemType);
            }
        }
    }
}

void verify_placement_new(ASTDiagnoser& linker, TypeLoc ptrType, Value* value) {
    switch(ptrType->kind()) {
        case BaseTypeKind::Pointer:{
            const auto child_type = ptrType->as_pointer_type_unsafe()->type;
            if (!child_type->satisfies(value, false)) {
                linker.error("value does not satisfy the pointer value type", value);
            }
            return;
        }
        case BaseTypeKind::Linked:{
            const auto linked = ptrType->as_linked_type_unsafe()->linked;
            if(linked->kind() == ASTNodeKind::TypealiasStmt) {
                verify_placement_new(linker, linked->as_typealias_unsafe()->actual_type, value);
                return;
            }
            break;
        }
        default:
            break;
    }
    linker.error("expected pointer value to be of pointer type", ptrType.encoded_location());
}

void TypeVerifier::VisitPlacementNewValue(PlacementNewValue *value) {
    RecursiveVisitor::VisitPlacementNewValue(value);
    // verify the type
    verify_placement_new(diagnoser, { value->pointer->getType(), value->pointer->encoded_location() }, value->value);
}

constexpr auto NonPublicDeclCallError = "calling a non-public function in a public generic declaration is not allowed, please use public/protected";
constexpr auto NonPublicDeclError = "non-public decl is being called in a public generic context, please use public/protected";

constexpr auto NonRetainedDeclCallError = "calling a non-retained function in a public generic declaration is not allowed, please use @retained annotation";
constexpr auto NonRetainedDeclError = "non-retained decl is being called in a public generic context, please use @retained annotation";

constexpr auto NonRetainedDeclCallComptimeError = "calling a non-retained function in a public comptime declaration is not allowed, please use @retained annotation";
constexpr auto NonRetainedDeclComptimeError = "non-retained decl is being called in a public comptime context, please use @retained annotation";

static FunctionCall* get_only_call(Value* value) {
    switch (value->kind()) {
        case ValueKind::FunctionCall:
            return value->as_func_call_unsafe();
        case ValueKind::AccessChain: {
            const auto c = value->as_access_chain_unsafe();
            return c->values.size() == 1 && c->values.back()->kind() == ValueKind::FunctionCall ? c->values.back()->as_func_call_unsafe() : nullptr;
        }
        default:
            return nullptr;
    }
}

// a non borrowing type is for example an integer, which can never point to freed memory
// a struct can contain a reference, which can point to freed memory
// so a reference/pointer or anything that can contain it is borrowing type
// when the function returns true, the type is non borrowing
// when returns false, the type may be borrowing (not 100%)
bool isNonBorrowingType(BaseType* type) {
    switch (type->kind()) {
        case BaseTypeKind::Bool:
        case BaseTypeKind::IntN:
        case BaseTypeKind::Double:
        case BaseTypeKind::Float:
        case BaseTypeKind::Float128:
        case BaseTypeKind::Void:
        case BaseTypeKind::NullPtr:
        case BaseTypeKind::Literal:
        case BaseTypeKind::LongDouble:
            return true;
        default:
            return false;
    }
}

void TypeVerifier::VisitFunctionCall(FunctionCall* call) {
    RecursiveVisitor<TypeVerifier>::VisitFunctionCall(call);
    if (is_generic_public_context) {
        const auto last_linked = call->parent_val->get_chain_last_linked();
        if (last_linked) {
            switch (last_linked->kind()) {
                case ASTNodeKind::FunctionDecl: {
                    if (last_linked->as_function_unsafe()->is_comptime()) {
                        break;
                    }
                    if (!is_linkage_public(last_linked->as_function_unsafe()->specifier())) {
                        diagnoser.error(call) << NonRetainedDeclCallError;
                        diagnoser.info(last_linked) << NonRetainedDeclError;
                    }
                    break;
                }
                case ASTNodeKind::GenericFuncDecl: {
                    if (last_linked->as_gen_func_decl_unsafe()->master_impl->is_comptime()) {
                        break;
                    }
                    if (!is_linkage_public(last_linked->as_gen_func_decl_unsafe()->master_impl->specifier())) {
                        diagnoser.error(call) << NonRetainedDeclCallError;
                        diagnoser.info(last_linked) << NonRetainedDeclError;
                    }
                    break;
                }
                case ASTNodeKind::GenericStructDecl: {
                    if (!is_linkage_public(last_linked->as_gen_struct_def_unsafe()->master_impl->specifier())) {
                        diagnoser.error(call) << NonRetainedDeclCallError;
                        diagnoser.info(last_linked) << NonRetainedDeclError;
                    }
                    break;
                }
                case ASTNodeKind::GenericUnionDecl: {
                    if (!is_linkage_public(last_linked->as_gen_union_decl_unsafe()->master_impl->specifier())) {
                        diagnoser.error(call) << NonRetainedDeclCallError;
                        diagnoser.info(last_linked) << NonRetainedDeclError;
                    }
                    break;
                }
                case ASTNodeKind::GenericVariantDecl: {
                    if (!is_linkage_public(last_linked->as_gen_variant_decl_unsafe()->master_impl->specifier())) {
                        diagnoser.error(call) << NonRetainedDeclCallError;
                        diagnoser.info(last_linked) << NonRetainedDeclError;
                    }
                    break;
                }
                case ASTNodeKind::VariantMember: {
                    const auto p = last_linked->as_variant_member_unsafe()->parent();
                    if (p->generic_parent && !p->generic_parent->as_gen_variant_decl_unsafe()->master_impl->is_body_retained()) {
                        diagnoser.error(call) << NonRetainedDeclCallError;
                        diagnoser.info(p) << NonRetainedDeclError;
                    }
                    break;
                }
            }
        }
    }
    if (is_public_comptime_context) {
        const auto last_linked = call->parent_val->get_chain_last_linked();
        if (last_linked) {
            switch (last_linked->kind()) {
                case ASTNodeKind::FunctionDecl: {
                    if (!last_linked->as_function_unsafe()->is_body_retained()) {
                        diagnoser.error(call) << NonRetainedDeclCallComptimeError;
                        diagnoser.info(last_linked) << NonRetainedDeclComptimeError;
                    }
                    break;
                }
                case ASTNodeKind::GenericFuncDecl: {
                    if (!last_linked->as_gen_func_decl_unsafe()->master_impl->is_body_retained()) {
                        diagnoser.error(call) << NonRetainedDeclCallComptimeError;
                        diagnoser.info(last_linked) << NonRetainedDeclComptimeError;
                    }
                    break;
                }
                case ASTNodeKind::GenericStructDecl: {
                    if (!last_linked->as_gen_struct_def_unsafe()->master_impl->is_body_retained()) {
                        diagnoser.error(call) << NonRetainedDeclCallComptimeError;
                        diagnoser.info(last_linked) << NonRetainedDeclComptimeError;
                    }
                    break;
                }
                case ASTNodeKind::GenericUnionDecl: {
                    if (!last_linked->as_gen_union_decl_unsafe()->master_impl->is_body_retained()) {
                        diagnoser.error(call) << NonRetainedDeclCallComptimeError;
                        diagnoser.info(last_linked) << NonRetainedDeclComptimeError;
                    }
                    break;
                }
                case ASTNodeKind::GenericVariantDecl: {
                    if (!last_linked->as_gen_variant_decl_unsafe()->master_impl->is_body_retained()) {
                        diagnoser.error(call) << NonRetainedDeclCallComptimeError;
                        diagnoser.info(last_linked) << NonRetainedDeclComptimeError;
                    }
                    break;
                }
                case ASTNodeKind::VariantMember: {
                    const auto p = last_linked->as_variant_member_unsafe()->parent();
                    if (p->generic_parent && !p->generic_parent->as_gen_variant_decl_unsafe()->master_impl->is_body_retained()) {
                        diagnoser.error(call) << NonRetainedDeclCallComptimeError;
                        diagnoser.info(p) << NonRetainedDeclComptimeError;
                    }
                    break;
                }
            }
        }
    }

    auto func_type = call->function_type();

    // check its not a function on a temporary destructible struct
    // lifetime check
    if (is_lifetime_check_enabled && call->parent_val->kind() == ValueKind::AccessChain) {
        const auto chain = call->parent_val->as_access_chain_unsafe();
        const auto total = chain->values.size();
        if (total > 1) {
            const auto grandparent = chain->values[total - 2];
            const auto gp_call = get_only_call(grandparent);
            if (gp_call) {
                const auto ty = gp_call->getType();
                const auto container = ty->get_members_container();
                if (container) {
                    const auto destr = container->destructor_func();
                    if (destr && func_type) {
                        // check if the return type's struct has lifetime params
                        // and the function has a return_lifetime annotation
                        const auto return_canon = func_type->returnType->canonical();
                        const auto return_container = return_canon->get_members_container();
                        bool has_lifetime = false;
                        if (return_container && return_container->kind() == ASTNodeKind::StructDecl) {
                            has_lifetime = !static_cast<StructDefinition*>(return_container)->lifetime_params.empty();
                        }
                        const auto linked_func = call->safe_linked_func();
                        const bool has_return_lifetime = linked_func && !linked_func->return_lifetime.empty();
                        if (has_lifetime && has_return_lifetime) {
                            diagnoser.error(call) << "function call on a temporary that is destroyed at expression end returns a struct with a lifetime dependency, please store the temporary in a variable";
                        }
                    }
                }
            }
        }
    }

    // type checking arguments
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

static bool is_empty_interface(ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::InterfaceDecl:
            if (node->as_interface_def_unsafe()->evaluated_nodes().empty()) return true;
            return false;
        case ASTNodeKind::GenericInterfaceDecl:
            if (node->as_gen_interface_decl_unsafe()->master_impl->evaluated_nodes().empty()) return true;
            return false;
        default:
            return false;
    }
}

void verify_interface_implementation(ImplementationsIndex& index, ASTDiagnoser& diagnoser, ImplDefinition* implementor, InterfaceDefinition* interface_non_master) {

    // so why always select the master (template) interface
    // because we need to get the base function, which has been indexed, to look up
    // we need to use the function pointers present in the master (template) interface
    const auto interface = interface_non_master->generic_parent != nullptr ? interface_non_master->generic_parent->as_gen_interface_decl_unsafe()->master_impl : interface_non_master;

    if(interface->is_extern() && interface->is_static()) {
        // compiler interfaces are extern and static
        return;
    }

    // First, verify inherited interfaces
    if (!interface->inherited.empty()) {
        const auto container = implementor->struct_type->get_members_container();
        const auto for_ = container ? ((ASTAny*) container) : ((ASTAny*) implementor->struct_type.getType());
        for (auto& inh : interface->inherited) {
            const auto canonical_node = inh.type->get_direct_linked_canonical_node();
            if (canonical_node == nullptr) continue;
            const auto impl = index.get_impl(canonical_node, for_);
            if (impl == nullptr) {
                // if (is_empty_interface(canonical_node)) continue;
                diagnoser.error(implementor) << "no implementation of interface '" << inh.type->representation() << "' could be found for '" << implementor->struct_type->representation() << '\'';
            }
        }
    }

    // Iterate over all functions in the interface
    for (auto& node : interface->evaluated_nodes()) {
        if (node->kind() == ASTNodeKind::FunctionDecl) {
            const auto func_node = node->as_function_unsafe();
            if(func_node->body.has_value()) {
                // default implementation available
                continue;
            }
            const auto found = implementor->implementation_of(func_node);
            if (found != nullptr) {
                // direct implementation present in impl
                continue;
            }
            diagnoser.error(implementor) << "type does not implement interface member '" << func_node->name_view() << "'";
            diagnoser.warn(func_node) << "function hasn't been implemented in an impl below";
        } else if (node->kind() == ASTNodeKind::GenericFuncDecl) {
            const auto gen_func = node->as_gen_func_decl_unsafe();
            if (gen_func->master_impl->body.has_value()) {
                // default implementation available
                continue;
            }
            // this calls specific method (implementation_of)
            const auto found = implementor->implementation_of(gen_func);
            if (found != nullptr) {
                // direct implementation present in impl
                continue;
            }
            diagnoser.error(implementor) << "type does not implement interface member '" << gen_func->master_impl->name_view() << "'";
            diagnoser.warn(gen_func) << "function hasn't been implemented in an impl below";
        } else {
            continue;
        }
        // we will do this when (someday) we support overriding implementations in interfaces
        // check for secondary default implementation in inherited
        // const auto secondary_impl = root_interface->any_child_function(func_node->name_view());
        // if (secondary_impl && secondary_impl->is_override() && secondary_impl->body.has_value()) {
        //     // secondary implementation present in impl
        //     continue;
        // }
    }
}

void TypeVerifier::VisitVarInitStmt(VarInitStatement *stmt) {
    if(!stmt->attrs.signature_resolved) {
        return;
    }
    auto& type = stmt->type;
    const auto value = stmt->value;
    if(type) {
        visit(type);
    }
    if(value) {
        visit(value);
    }
    if(type && value && !stmt->type->satisfies(value, false)) {
        unsatisfied_type_err(diagnoser, allocator, value, type);
    }
    if(stmt->known_type()->kind() == BaseTypeKind::Void) {
        diagnoser.error(stmt) << "variable with name '" << stmt->name_view() << "' type can't be of type void";
    }
    if (stmt->is_top_level()) {
        // check var init is non-destructible type
        if(stmt->is_never_destructed() == false && stmt->known_type()->get_destructor() != nullptr) {
            diagnoser.error(stmt) << "top level variables or constants must be non-destructible, or must use @never_destructed annotation";
        }
    }
}

void type_verify(ImplementationsIndex& index, ASTDiagnoser& diagnoser, ASTAllocator& allocator, std::span<ASTNode*> nodes) {
    TypeVerifier verifier(index, allocator, diagnoser);
    for(const auto node : nodes) {
        verifier.visit(node);
    }
}

void TypeVerifier::VisitImplDecl(ImplDefinition* implDecl) {
    RecursiveVisitor::VisitImplDecl(implDecl);
    if (implDecl->interface_type) {
        const auto interface_node = implDecl->interface_type->get_direct_linked_canonical_node();
        if (interface_node->kind() == ASTNodeKind::InterfaceDecl) {
            const auto interface = interface_node->as_interface_def_unsafe();
            verify_interface_implementation(index, diagnoser, implDecl, interface);
        }
    }
}

void TypeVerifier::VisitIfStmt(IfStatement *stmt) {
    if (stmt->computed_scope.has_value()) {
        const auto scope = stmt->computed_scope.value();
        if (scope) {
            visit(scope);
        }
        return;
    }
    RecursiveVisitor::VisitIfStmt(stmt);
}

void TypeVerifier::VisitAssignmentStmt(AssignStatement *assign) {

    RecursiveVisitor::VisitAssignmentStmt(assign);

    const auto lhs = assign->lhs;
    const auto value = assign->value;
    const auto lhsType = lhs->getType();

    // check if operator is overloaded
    // direct assignment cannot be overloaded
    if(assign->assOp != Operation::Assignment) {
        const auto can_node = lhsType->get_linked_canonical_node(true, false);
        if(can_node) {
            const auto container = can_node->get_members_container();
            if(container) {
                // operator is overloaded, currently no check is being performed here
                return;
            }
        }
    }

    // check assignment satisfies the lhs type
    switch(assign->assOp){
        case Operation::Assignment:
            if (!lhsType->satisfies(value, true)) {
                unsatisfied_type_err(diagnoser, value, lhsType);
            }
            break;
        case Operation::Addition:
        case Operation::Subtraction:
            if(lhsType->kind() == BaseTypeKind::Pointer) {
                const auto rhsType = value->getType()->canonical();
                if(rhsType->kind() != BaseTypeKind::IntN) {
                    unsatisfied_type_err(diagnoser, value, lhsType);
                }
            } else if (!lhsType->satisfies(value, true)) {
                unsatisfied_type_err(diagnoser, value, lhsType);
            }
            break;
        default:
            break;
    }

}

void TypeVerifier::VisitReturnStmt(ReturnStatement *node) {
    RecursiveVisitor::VisitReturnStmt(node);
    auto& value = node->value;
    if (value) {
        const auto func_type = current_func_type;
        if(func_type->data.signature_resolved && func_type->returnType) {
            const auto func = func_type->as_function();
            if(func && func->is_constructor_fn()) {
                return;
            }
            const auto implicit = func_type->returnType->implicit_constructor_for(value);
            if (implicit &&
                // this check means current function is not the implicit constructor we're trying to link for value
                // basically an implicit constructor can has a value returned of a type for which it's an implicit constructor of (in comptime)
                implicit != func_type &&
                // this check means current function's parent (if it's inside a struct) is not the same parent as the implicit constructor parent
                // meaning implicit constructor and the function that's going to use the implicit constructor can't be inside same container
                (func && func->parent() != implicit->parent())
            ) {
                // TODO: handle implicit constructor
                return;
            }
            if(!func_type->returnType->satisfies(value, false)) {
                unsatisfied_type_err(diagnoser, value, func_type->returnType);
            }
        }
    } else {
        const auto func_type = current_func_type;
        if(func_type->returnType && func_type->returnType->kind() != BaseTypeKind::Void) {
            diagnoser.error(node) << "function expects a non void return of type '" << func_type->returnType->representation() << "'";
        }
    }

}

void TypeVerifier::VisitFunctionDecl(FunctionDeclaration *decl) {
    // visiting the signature of the function
    for(auto param : decl->params) {
        // default values aren't verified during link signature
        // because they may not have been linked at that time
        if(param->defValue) {
            const auto imp_constructor = param->type->implicit_constructor_for(param->defValue);
            if(imp_constructor == nullptr && !param->type->satisfies(param->defValue, false)) {
                unsatisfied_type_err(diagnoser, param->defValue, param->type);
            }
        }
    }
    const auto prev_pub_comptime = is_public_comptime_context;
    is_public_comptime_context = prev_pub_comptime || (decl->is_comptime() && is_linkage_public(decl->specifier()));

    const auto prev = current_func_type;
    current_func_type = decl;

    RecursiveVisitor::VisitFunctionDecl(decl);

    current_func_type = prev;
    is_public_comptime_context = prev_pub_comptime;

}

void TypeVerifier::VisitLambdaFunction(LambdaFunction *func) {
    const auto prev = current_func_type;
    current_func_type = func;
    RecursiveVisitor::VisitLambdaFunction(func);
    current_func_type = prev;
}

void TypeVerifier::VisitLinkedType(LinkedType* type) {
    if (is_generic_public_context) {
        // TODO: errors aren't being reported properly because linked types don't store location at the moment
        const auto errStr = std::string_view("using a non-public type in a public generic declaration is not allowed, please use public / protected");
        switch (type->linked->kind()) {
            case ASTNodeKind::GenericStructDecl:
                if (!is_linkage_public(type->linked->as_gen_struct_def_unsafe()->master_impl->specifier())) {
                    diagnoser.error(type->linked) << errStr;
                }
                break;
            case ASTNodeKind::GenericVariantDecl:
                if (!is_linkage_public(type->linked->as_gen_variant_decl_unsafe()->master_impl->specifier())) {
                    diagnoser.error(type->linked) << errStr;
                }
                break;
            case ASTNodeKind::GenericUnionDecl:
                if (!is_linkage_public(type->linked->as_gen_union_decl_unsafe()->master_impl->specifier())) {
                    diagnoser.error(type->linked) << errStr;
                }
                break;
            case ASTNodeKind::GenericInterfaceDecl:
                if (!is_linkage_public(type->linked->as_gen_interface_decl_unsafe()->master_impl->specifier())) {
                    diagnoser.error(type->linked) << errStr;
                }
                break;
            default:
                return;
        }
    }
}

void TypeVerifier::VisitGenericFuncDecl(GenericFuncDecl* node) {
    const auto prev_gen_public = is_generic_public_context;
    is_generic_public_context = node->master_impl->specifier() == AccessSpecifier::Public;
    visit(node->master_impl);
    is_generic_public_context = prev_gen_public;
}

void TypeVerifier::VisitGenericTypeDecl(GenericTypeDecl* node) {
    const auto prev_gen_public = is_generic_public_context;
    is_generic_public_context = node->master_impl->specifier() == AccessSpecifier::Public;
    visit(node->master_impl);
    is_generic_public_context = prev_gen_public;
}

void TypeVerifier::VisitGenericStructDecl(GenericStructDecl* node) {
    const auto prev_gen_public = is_generic_public_context;
    is_generic_public_context = node->master_impl->specifier() == AccessSpecifier::Public;
    visit(node->master_impl);
    is_generic_public_context = prev_gen_public;
}

void TypeVerifier::VisitGenericUnionDecl(GenericUnionDecl* node) {
    const auto prev_gen_public = is_generic_public_context;
    is_generic_public_context = node->master_impl->specifier() == AccessSpecifier::Public;
    visit(node->master_impl);
    is_generic_public_context = prev_gen_public;
}

void TypeVerifier::VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
    const auto prev_gen_public = is_generic_public_context;
    is_generic_public_context = node->master_impl->specifier() == AccessSpecifier::Public;
    visit(node->master_impl);
    is_generic_public_context = prev_gen_public;
}

void TypeVerifier::VisitGenericVariantDecl(GenericVariantDecl* node) {
    const auto prev_gen_public = is_generic_public_context;
    is_generic_public_context = node->master_impl->specifier() == AccessSpecifier::Public;
    visit(node->master_impl);
    is_generic_public_context = prev_gen_public;
}

void TypeVerifier::VisitGenericImplDecl(GenericImplDecl* node) {
    const auto prev_gen_public = is_generic_public_context;
    is_generic_public_context = node->master_impl->specifier() == AccessSpecifier::Public;
    visit(node->master_impl);
    is_generic_public_context = prev_gen_public;
}

bool* get_flag(TypeVerifier& verifier, const chem::string_view& name) {
    constexpr auto hasher = std::hash<chem::string_view>();
    switch (hasher(name)) {
        case hasher("lifetime_check"):
            return &verifier.is_lifetime_check_enabled;
        default:
            return nullptr;
    }
}

void TypeVerifier::VisitUnsafeBlock(UnsafeBlock* block) {
    // handle flag toggling
    if(!block->flag_name.empty()) {
        const auto flag = get_flag(*this, block->flag_name);
        if (flag == nullptr) {
            diagnoser.warn(block) << "couldn't find flag";
            RecursiveVisitor<TypeVerifier>::VisitUnsafeBlock(block);
            return;
        }
        const auto prev = *flag;
        *flag = block->flag_value;
        RecursiveVisitor<TypeVerifier>::VisitUnsafeBlock(block);
        *flag = prev;
    } else {
        RecursiveVisitor<TypeVerifier>::VisitUnsafeBlock(block);
    }
}