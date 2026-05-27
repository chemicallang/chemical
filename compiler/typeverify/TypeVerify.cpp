
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
#include "ast/structures/GenericTypeParameter.h"
#include "ast/base/InterfaceBits.h"
#include "ast/utils/ASTUtils.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/symres/SymbolResolver.h"
#include "core/source/LocationManager.h"

static GenericTypeParameter* get_generic_param(BaseType* type) {
    if(type->kind() == BaseTypeKind::Linked) {
        auto linked = type->as_linked_type_unsafe()->linked;
        if(linked && linked->kind() == ASTNodeKind::GenericTypeParam) {
            return linked->as_generic_type_param_unsafe();
        }
    }
    return nullptr;
}

void verify_container_inherited(TypeVerifier& verifier, MembersContainer* container) {
    auto& inherited = container->inherited;
    if (!inherited.empty()) {
        auto& diagnoser = verifier.diagnoser;
        // first type can be a struct or interface (shouldn't be variant)
        auto& first_node_type = inherited.front().type;
        const auto first_node = first_node_type->get_direct_linked_canonical_node();
        if (first_node->kind() != ASTNodeKind::StructDecl && first_node->kind() != ASTNodeKind::GenericStructDecl) {
            diagnoser.error(first_node_type.encoded_location()) << "the type in inheritance list must be a struct";
        }
        if (inherited.size() > 1) {
            // all other types must be interfaces or empty struct types
            auto start = inherited.data() + 1;
            const auto end = inherited.data() + inherited.size();
            while (start != end) {
                const auto node = start->type->get_direct_linked_canonical_node();
                if (node->kind() == ASTNodeKind::StructDecl) {
                    // check if struct is empty, if empty, we allow it, otherwise error out
                    const auto structDecl = node->as_struct_def_unsafe();
                    if (!structDecl->is_sizeof_zero) {
                        diagnoser.error(start->type.encoded_location()) << "struct type is not empty (contains variables) being inherited (not in the first position) in inheritance list";
                    }
                } else if (node->kind() == ASTNodeKind::GenericStructDecl) {
                    // check if struct is empty, if empty, we allow it, otherwise error out
                    const auto structDecl = node->as_gen_struct_def_unsafe();
                    if (!structDecl->master_impl->is_sizeof_zero) {
                        diagnoser.error(start->type.encoded_location()) << "struct type is not empty (contains variables) being inherited (not in the first position) in inheritance list";
                    }
                } else {
                    diagnoser.error(start->type.encoded_location()) << "the type in inheritance list must be a struct";
                }
                start++;
            }
        }
    }
}

inline static bool is_mutable_ref_type(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Reference:
            return type->as_reference_type_unsafe()->is_mutable;
        default:
            return false;
    }
}

void verify_mutation(TypeVerifier& verifier, Value* lhs) {
    // get the first value in chain, check if its a struct member
    // if it is, assignment to struct member requires mutable self reference
    const auto lhs_chain = lhs->as_chain_value();
    if(lhs_chain) {
        const auto first_chain_val = get_first_chain_id(lhs_chain);
        if (first_chain_val) {
            const auto linked = first_chain_val->linked;
            if(linked->kind() == ASTNodeKind::StructMember) {
                // check current function has a mutable self reference
                const auto curr_func = verifier.current_func_type;
                const auto decl = curr_func->as_function();
                if(decl && !decl->is_constructor_fn()) {
                    const auto self_param = decl->get_self_param();
                    if(self_param == nullptr || !is_mutable_ref_type(self_param->type)) {
                        verifier.diagnoser.error("mutating a struct member requires a mutable self reference", lhs);
                    }
                }
            }
        }
    }
}

void TypeVerifier::VisitStructDecl(StructDefinition* def) {
    RecursiveVisitor::VisitStructDecl(def);
    verify_container_inherited(*this, def);
}

void TypeVerifier::VisitUnionDecl(UnionDef* def) {
    RecursiveVisitor::VisitUnionDecl(def);
    verify_container_inherited(*this, def);
}

void TypeVerifier::VisitVariantDecl(VariantDefinition* def) {
    RecursiveVisitor::VisitVariantDecl(def);
    verify_container_inherited(*this, def);
}

void TypeVerifier::VisitInterfaceDecl(InterfaceDefinition* interface) {
    RecursiveVisitor::VisitInterfaceDecl(interface);
    // ensure that every inherited type after the first inherited type is an interface
    // this makes the inheritance list predictable
    // if this is an interface decl, every inherited type must be an interface
    for(auto& inherits : interface->inherited) {
        const auto node = inherits.type->get_direct_linked_canonical_node();
        if (node->kind() != ASTNodeKind::InterfaceDecl && node->kind() != ASTNodeKind::GenericInterfaceDecl) {
            diagnoser.error(inherits.type.encoded_location()) << "interfaces can only inherit interfaces";
        }
    }
}

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

void TypeVerifier::VisitIncDecValue(IncDecValue* value) {
    RecursiveVisitor::VisitIncDecValue(value);
    // check if we can modify the value
    verify_mutation(*this, value);
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

bool check_chain_mutability(TypeVerifier& verifier, const std::vector<Value*>& values, int end_index) {
    for (int i = end_index; i >= 0; --i) {
        auto val = values[i];

        BaseType* type = val->getType();
        if (type && type->kind() == BaseTypeKind::Reference) {
            // It's a reference, so it dictates mutability of the chain up to this point
            return type->as_reference_type_unsafe()->is_mutable;
        }

        if (type && type->kind() == BaseTypeKind::Pointer) {
            return true;
        }

        // It is a value (struct, field, etc.)
        if (!val->check_is_mutable(false)) {
            return false;
        }

        // If this is the root of the chain (and not a reference/pointer),
        // effectively meaning we are accessing a member of 'self' explicitly or implicitly
        // or a local variable.
        // check_is_mutable handles local variables (if 'let' vs 'var').
        // But for StructMember, check_is_mutable just sees the member definition.
        // We need to check if we are accessing a member of 'self', and if 'self' is mutable.
        if (i == 0) {
            const auto linked = val->get_chain_last_linked();
            if (linked && linked->kind() == ASTNodeKind::StructMember) {
                // Must be implicit self access
                const auto curr_func_type = verifier.current_func_type;
                if (curr_func_type) {
                    const auto func_decl = curr_func_type->as_function();
                    if (func_decl && !func_decl->is_constructor_fn()) {
                        const auto self_param = func_decl->get_self_param();
                        // If self is present and NOT mutable, then implicit member access is immutable
                        if (self_param && !is_mutable_ref_type(self_param->type)) {
                            return false;
                        }
                    }
                }
            }
        }

        // Even if this value is mutable (e.g. a field that is not const),
        // its mutability depends on its parent (container).
        // So we continue to the next iteration (parent).
    }
    return true; // Reached root and it passed requirements (or was handled).
}

void verify_call_mutability(TypeVerifier& verifier, FunctionCall* call) {
    auto& diagnoser = verifier.diagnoser;
    const auto parent_val = call->parent_val;
    const auto linked = parent_val->get_chain_last_linked();
    // enum member being used as a no value
    const auto linked_kind = linked ? linked->kind() : ASTNodeKind::EnumMember;
    const auto func_decl = linked_kind == ASTNodeKind::FunctionDecl ? linked->as_function_unsafe() : nullptr;
    if(func_decl) {
//        if(func_decl->is_unsafe() && resolver.safe_context) {
//            resolver.error("unsafe function with name should be called in an unsafe block", this);
//        }
        const auto self_param = func_decl->get_self_param();
        if(self_param) {
            const auto grandpa = get_parent_from(parent_val);
            if(grandpa) {
                if(!self_param->type->is_mutable()) {
                    // if self param is immutable, we don't need to check anything
                } else {
                    bool is_mutable = true;
                    if(parent_val->kind() == ValueKind::AccessChain) {
                        auto chain = parent_val->as_access_chain_unsafe();
                        if(chain->values.size() >= 2) {
                            if(!check_chain_mutability(verifier, chain->values, (int)chain->values.size() - 2)) {
                                is_mutable = false;
                            }
                        } else {
                            // Should not happen if grandpa exists (implies at least 2 elements: obj.func)
                        }
                    } else {
                        const auto first_value = get_first_chain_value(parent_val);
                        // Here we should also use check_chain_mutability if possible, but get_first_chain_value returns one node.
                        // However, parent_val here IS get_grandpa_from(parent_val) effectively?
                        // Actually, if parent_val is NOT AccessChain, it must be Identifier (p).
                        // p is linked to member.
                        // We check if p is mutable (var p). It is.
                        // But we also need to check if SELF is mutable.
                        // Wait, if parent_val is Identifier p. It is linked to StructMember.
                        // check_is_mutable on StructMember 'p' returns true.
                        // So checking first_value->check_is_mutable is insufficient for implicit self.
                        // We must mimic check_chain_mutability logic here for single value.

                        if(first_value) {
                             if (!first_value->check_is_mutable(false)) {
                                 is_mutable = false;
                             } else {
                                const auto linked = first_value->get_chain_last_linked();
                                if (linked && linked->kind() == ASTNodeKind::StructMember) {
                                     // Implicit self check
                                    const auto curr_func_type = verifier.current_func_type;
                                    if (curr_func_type) {
                                        const auto func_decl = curr_func_type->as_function();
                                        if (func_decl && !func_decl->is_constructor_fn()) {
                                            const auto sp = func_decl->get_self_param();
                                            if (sp && !is_mutable_ref_type(sp->type)) {
                                                is_mutable = false;
                                            }
                                        }
                                    }
                                }
                             }
                        }
                    }

                    if(!is_mutable) {
                        diagnoser.error("call requires a mutable implicit self argument, however current self argument is not mutable", call);
                    }
                }
            } else {
                const auto curr_func_type = verifier.current_func_type;
                if (curr_func_type == nullptr) return;
                const auto arg_self = curr_func_type->get_self_param();
                if (!arg_self) {
                    diagnoser.error("cannot call function without an implicit self arg which is not present", call);
                } else if (self_param->type->is_mutable() && !arg_self->type->is_mutable()) {
                    diagnoser.error("call requires a mutable implicit self argument, however current self argument is not mutable", call);
                }
            }
        }
    }
}

void TypeVerifier::VisitFunctionCall(FunctionCall* call) {
    RecursiveVisitor<TypeVerifier>::VisitFunctionCall(call);
    // verifying the call mutability (self param is mutable and all that...)
    verify_call_mutability(*this, call);
    // this verifies comptimeness of arguments
    // TODO: this verifyArguments need to be moved to this .cpp file (make static)
    const auto curr_func_type = current_func_type;
    if(curr_func_type) {
        call->verifyArguments(diagnoser, curr_func_type);
    }
    // verify retention
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
                default:
                    break;
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
                default:
                    break;
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

    // first we check if the value is mutable
    // immutable values cannot be used (even in operator overloading)
    if(!lhs->check_is_mutable(true)) {
        diagnoser.error("cannot assign to a non mutable value", lhs);
    }

    // get the first value in chain, check if its a struct member
    // if it is, assignment to struct member requires mutable self reference
    verify_mutation(*this, lhs);

    // check assignment satisfies the lhs type
    switch(assign->assOp){
        case Operation::Assignment:
            if (!lhsType->satisfies(value, true)) {
                unsatisfied_type_err(diagnoser, value, lhsType);
            }
            // check generic type params have Copy bit set
            {
                auto param = get_generic_param(lhsType);
                if(param && !param->current_bits.has(InterfaceBits::COPY_BIT)) {
                    diagnoser.error(assign) << "generic type '" << param->identifier << "' that is not 'Copy' cannot be implicitly copied";
                }
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