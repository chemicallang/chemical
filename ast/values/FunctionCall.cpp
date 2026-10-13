// Copyright (c) Chemical Language Foundation 2025.

#include "FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ReferenceOfValue.h"
#include "ast/values/LambdaFunction.h"
#include "ast/utils/ASTUtils.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/values/StructValue.h"
#include "ast/values/NullValue.h"
#include "ast/base/BaseType.h"
#include "ast/base/TypeBuilder.h"
#include "ast/types/GenericType.h"
#include "ast/types/IntNType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/LinkedType.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/CapturedComptimeVariable.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/DynamicType.h"
#include "ast/values/DynamicValue.h"
#include "ast/values/PointerValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/Scope.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/FileScope.h"
#include <cstdio>


uint64_t FunctionCall::byte_size(const TargetData& target) {
    return getType()->byte_size(target);
}

FunctionType* FunctionCall::func_type_from_parent_type(BaseType* can_type) {
    if(can_type->kind() == BaseTypeKind::CapturingFunction) {
        return can_type->as_capturing_func_type_unsafe()->func_type->as_function_type();
    }
    if(can_type->kind() == BaseTypeKind::Reference) {
        const auto refType = can_type->as_reference_type_unsafe()->type->canonical();
        if(refType->kind() == BaseTypeKind::CapturingFunction) {
            return refType->as_capturing_func_type_unsafe()->func_type->as_function_type();
        }
    }
    auto func_type = can_type->get_canonical_function_type();
    return func_type;
}

FunctionType* FunctionCall::function_type() {
    if(!parent_val) return nullptr;
    const auto type = parent_val->getType();
    if(!type) return nullptr;
    const auto can_type = type->canonical();
    return func_type_from_parent_type(can_type);
}

FunctionType* FunctionCall::get_function_type_during_linking() {
    if(!parent_val) return nullptr;
    const auto type = parent_val->getType();
    if(!type) return nullptr;
    const auto can_type = type->canonical();
    if(can_type->kind() == BaseTypeKind::CapturingFunction) {
        return can_type->as_capturing_func_type_unsafe()->func_type->as_function_type();
    }
    if(can_type->kind() == BaseTypeKind::Reference) {
        const auto refType = can_type->as_reference_type_unsafe()->type->canonical();
        if(refType->kind() == BaseTypeKind::CapturingFunction) {
            return refType->as_capturing_func_type_unsafe()->func_type->as_function_type();
        }
    }
    if(can_type->kind() == BaseTypeKind::Linked || can_type->kind() == BaseTypeKind::Generic) {
        const auto container = can_type->get_direct_linked_canonical_node();
        if(container) {
            switch(container->kind()) {
                case ASTNodeKind::StructDecl:{
                    // linked with master implementation
                    return container->as_struct_def_unsafe()->constructor_func(values);
                }
                case ASTNodeKind::VariantDecl: {
                    // linked with master implementation
                    return container->as_variant_def_unsafe()->constructor_func(values);
                }
                default:
                    break;
            }
        }
    }
    return can_type->as_function_type();;
}

FunctionType* FunctionCall::known_func_type() {
    auto decl = safe_linked_func();
    if(decl) {
        return decl->as_function_type();
    }
    auto func_type = parent_val->getType();
    if(func_type->as_function_type()) {
        return (FunctionType*) func_type;
    } else {
        return nullptr;
    }
}

BaseType* FunctionCall::get_arg_type(unsigned int index) {
    auto func_type = parent_val->getType()->as_function_type();
    auto param = func_type->func_param_for_arg_at(index);
    return param->type;
}


void FunctionCall::infer_generic_args(ASTAllocator& allocator, ASTDiagnoser& diagnoser, std::vector<TypeLoc>& inferred) {
    const auto func_type = known_func_type();
    if(func_type) {
        const auto func = func_type;
        // going over function parameters to see which arguments have been given, if they do have a generic type
        // going over only explicit function params
        auto arg_offset = func->explicit_func_arg_offset();
        if(func->isExtensionFn()) {
            auto selfParam = func->get_self_param();
            assert(selfParam != nullptr);
            const auto param_type = selfParam->type;
            const auto grandpa = get_parent_from(parent_val);
            if(grandpa) {
                const auto arg_type = grandpa->getType();
                if(arg_type) {
                    infer_types_by_args(diagnoser, parent_val->linked_node(), generic_list.size(), param_type, {arg_type, grandpa->encoded_location()}, inferred, this);
                }
            }
        }
        const auto values_size = values.size();
        // guard against calls whose argument list is longer than the parameter
        // list (e.g. a member call where `values` includes the receiver)
        while(arg_offset < values_size && arg_offset < func->params.size()) {
            const auto param = func->params[arg_offset];
            const auto param_type = param->type;
            const auto arg_type_loc = values[arg_offset]->encoded_location();
            const auto arg_type = values[arg_offset]->getType();
            if(!arg_type) {
#ifdef DEBUG
                diagnoser.error(this) << "couldn't get arg type " << values[arg_offset]->representation() << " in function call " << representation();
#endif
                arg_offset++;
                continue;
            }
            infer_types_by_args(diagnoser, parent_val->linked_node(), generic_list.size(), param_type, {arg_type, arg_type_loc}, inferred, this);
            arg_offset++;
        }
    } else {
        const auto linked = parent_val->linked_node();
        if(linked->kind() == ASTNodeKind::VariantMember) {
            const auto member = linked->as_variant_member_unsafe();
            const auto values_size = values.size();
            auto i = 0;
            while(i < values_size) {
                const auto param_type = (member->values.begin() + i)->second->type;
                const auto arg_type_loc = values[i]->encoded_location();
                const auto arg_type = values[i]->getType();
                if(arg_type) {
                    infer_types_by_args(diagnoser, member->parent(), generic_list.size(), param_type, {arg_type, arg_type_loc}, inferred, this);
                }
                i++;
            }
        }
    }
}

void FunctionCall::infer_return_type(ASTDiagnoser& diagnoser, std::vector<TypeLoc>& inferred, BaseType* expected_type) {
    const auto func = safe_linked_func();
    if(!func || expected_type->kind() == BaseTypeKind::Any) return;
    infer_types_by_args(diagnoser, func, generic_list.size(), func->returnType, {expected_type, ZERO_LOC}, inferred, this);
}

ASTNode *FunctionCall::linked_node() {
    const auto known = getType();
    return known ? known->linked_node() : nullptr;
}


void constructor_not_found_err(Diag& diag, ExtendableMembersContainerNode* parent_struct, FunctionCall* call) {
    diag << "struct with name " << parent_struct->name_view() << " doesn't have a constructor ";
    if (call->values.size() == 1) {
        diag << "for type '";
        diag << call->values[0]->getType()->representation();
        diag << "'";
    } else {
        diag << "that satisfies given arguments " << call->representation();
    }
}

void link_constructor_id(VariableIdentifier* parent_id, ASTAllocator& allocator, GenericInstantiatorAPI& genApi, FunctionCall* call, bool specialize_generic, InstantiationRequirement requirement) {
    if(!parent_id->linked) return;
    const auto linked_kind = parent_id->linked->kind();
    switch(linked_kind) {
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl: {
            const auto parent_struct = parent_id->linked->as_extendable_members_container_unsafe();
            auto constructorFunc = parent_struct->constructor_func(call->values);
            if(constructorFunc) {
                parent_id->linked = constructorFunc;
                parent_id->setType(constructorFunc->known_type());
            } else {
                constructor_not_found_err(genApi.getDiagnoser().error(parent_id), parent_struct, call);
            }
            return;
        }
        case ASTNodeKind::GenericStructDecl:{
            const auto gen_struct = parent_id->linked->as_gen_struct_def_unsafe();
            if(!specialize_generic) return;
            const auto parent_struct = gen_struct->instantiate_type(genApi, call->generic_list, call->encoded_location(), requirement);
            if(parent_struct == nullptr) return;
            auto constructorFunc = parent_struct->constructor_func(call->values);
            if(constructorFunc) {
                parent_id->linked = constructorFunc;
                parent_id->setType(constructorFunc->known_type());
            } else {
                constructor_not_found_err(genApi.getDiagnoser().error(parent_id), parent_struct, call);
            }
            return;
        }
        case ASTNodeKind::GenericVariantDecl: {
            const auto gen_struct = parent_id->linked->as_gen_variant_decl_unsafe();
            if(!specialize_generic) return;
            const auto parent_struct = gen_struct->instantiate_type(genApi, call->generic_list, call->encoded_location(), requirement);
            if(parent_struct == nullptr) return;
            auto constructorFunc = parent_struct->constructor_func(call->values);
            if(constructorFunc) {
                parent_id->linked = constructorFunc;
                parent_id->setType(constructorFunc->known_type());
            } else {
                constructor_not_found_err(genApi.getDiagnoser().error(parent_id), parent_struct, call);
            }
            return;
        }
        default: {
            return;
        }
    }
}

// the returned generic iteration is the previous iteration of the struct of which constructor we linked with
// when this method is called, it automatically register the generic arguments with the struct constructor getting the new iteration and setting it active
void FunctionCall::link_constructor(ASTAllocator& allocator, GenericInstantiatorAPI& genApi, bool specialize_generic, InstantiationRequirement requirement) {
    // relinking parent with constructor of the struct
    // if it's linked with struct
    const auto parent_kind = parent_val->val_kind();
    switch(parent_kind) {
        case ValueKind::Identifier:{
            const auto parent_id = parent_val->as_identifier_unsafe();
            link_constructor_id(parent_id, allocator, genApi, this, specialize_generic, requirement);
            return;
        }
        case ValueKind::AccessChain:{
            const auto parent_chain = parent_val->as_access_chain_unsafe();
            const auto last = parent_chain->values.back()->as_identifier();
            if(last) {
                link_constructor_id(last, allocator, genApi, this, specialize_generic, requirement);
                // if the type has been updated, we should update of chain as well
                parent_chain->setType(last->getType());
                return;
            } else {
                return;
            }
        }
        default:
            return;
    }
}

inline VariableIdentifier* get_parent_id(Value* value) {
    return value->get_last_id();
}

void setParentType(Value* parent_val, BaseType* type) {
    switch(parent_val->kind()) {
        case ValueKind::Identifier:
        case ValueKind::FunctionCall:
        case ValueKind::IndexOperator:
            parent_val->setType(type);
            return;
        case ValueKind::AccessChain:
            setParentType(parent_val->as_access_chain_unsafe()->values.back(), type);
            parent_val->setType(type);
            return;
        default:
            return;
    }
}

bool FunctionCall::instantiate_gen_call(GenericInstantiatorAPI& genApi, BaseType* expected_type, InstantiationRequirement requirement) {
    // relinking generic decl
    const auto parent_id = get_parent_id(parent_val);
    if(!parent_id) return true;

    const auto linked = parent_id->linked;
    switch(linked->kind()) {
        case ASTNodeKind::GenericFuncDecl:{
            const auto gen_decl = linked->as_gen_func_decl_unsafe();
            auto new_link = gen_decl->instantiate_call(genApi, this, expected_type, requirement);
            if(!new_link) {
                return false;
            }
            parent_id->linked = new_link;
            setParentType(parent_val, new_link->known_type());
            return true;
        }
        case ASTNodeKind::VariantMember:{
            const auto mem = linked->as_variant_member_unsafe();
            const auto mem_gen = mem->parent()->generic_parent;
            if(mem_gen != nullptr) {
                const auto new_link = mem_gen->as_gen_variant_decl_unsafe()->instantiate_call(genApi, this, expected_type, requirement);
                if(!new_link) {
                    return false;
                }
                const auto var_id = get_parent_from(parent_val);
                if(var_id) {
                    // every variable is like this MyVariant.Member() <-- get parent from parent is MyVariant
                    var_id->as_identifier()->linked = new_link;
                }
                const auto new_mem = new_link->direct_child(mem->name)->as_variant_member_unsafe();
                parent_id->linked = new_mem;
                setParentType(parent_val, new_mem->known_type());
            }
            return true;
        }
        case ASTNodeKind::GenericStructDecl: {
            const auto gen_struct = linked->as_gen_struct_def_unsafe();
            const auto parent_struct = gen_struct->instantiate_type(genApi, generic_list, encoded_location(), requirement);
            if(parent_struct == nullptr) return false;
            auto constructorFunc = parent_struct->constructor_func(values);
            if(constructorFunc) {
                parent_id->linked = constructorFunc;
                setParentType(parent_val, constructorFunc->known_type());
                return true;
            } else {
                constructor_not_found_err(genApi.getDiagnoser().error(parent_id), parent_struct, this);
                return false;
            }
        }
        case ASTNodeKind::GenericVariantDecl: {
            const auto gen_struct = linked->as_gen_variant_decl_unsafe();
            const auto parent_struct = gen_struct->instantiate_type(genApi, generic_list, encoded_location(), requirement);
            if(parent_struct == nullptr) return false;
            auto constructorFunc = parent_struct->constructor_func(values);
            if(constructorFunc) {
                parent_id->linked = constructorFunc;
                setParentType(parent_val, constructorFunc->known_type());
                return true;
            } else {
                constructor_not_found_err(genApi.getDiagnoser().error(parent_id), parent_struct, this);
                return false;
            }
        }
        default:
            return true;

    }
}

bool FunctionCall::primitive() {
    return false;
}

bool FunctionCall::compile_time_computable() {
    auto func = safe_linked_func();
    return func && func->is_comptime();
}

Value *FunctionCall::find_in(InterpretScope &scope, Value *parent) {
    auto id = parent->as_identifier();
    if(id != nullptr) {
        return parent->call_member(scope, id->value, values);
    } else {
        scope.error("No identifier for function call", this);
        return nullptr;
    }
}

/**
 * Find the concrete implementation of an interface method for a given struct type.
 * Searches through the struct's parent scope (FileScope, Namespace) for ImplDefinitions
 * that implement the given interface for the given struct, and returns the matching
 * function by name.
 */
static FunctionDeclaration* find_interface_impl_fn(
    InterfaceDefinition* interfaceDef,
    ExtendableMembersContainerNode* structDef,
    const chem::string_view& funcName
) {
    // Helper lambda to search a vector of ASTNode* for matching ImplDefinitions
    auto searchNodes = [&](const std::vector<ASTNode*>& nodes) -> FunctionDeclaration* {
        for (auto child : nodes) {
            if (!child || child->kind() != ASTNodeKind::ImplDecl) continue;
            auto impl = child->as_impl_def_unsafe();
            if (!impl || !impl->struct_type) continue;
            auto implStruct = impl->struct_type->get_direct_linked_canonical_node();
            bool structMatches = (implStruct == structDef);
            if (!implStruct || !structMatches) {
                // The same canonical struct can have different pointer representations
                // when accessed through different resolution paths. Fall back to name
                // comparison in this case.
                if(!structMatches && implStruct && structDef &&
                    implStruct->get_node_identifier() == structDef->get_node_identifier()) {
                    structMatches = true;
                }
                if (!structMatches) {
                    continue;
                }
            }
            if (!impl->interface_type) continue;
            auto implInterfaceNode = impl->interface_type->get_direct_linked_node();
            bool interfaceMatches = (implInterfaceNode == interfaceDef);
            if (!interfaceMatches && implInterfaceNode && interfaceDef &&
                implInterfaceNode->get_node_identifier() == interfaceDef->get_node_identifier()) {
                interfaceMatches = true;
            }
            if (!interfaceMatches) continue;
            auto fn = impl->direct_child_function(funcName);
            if (fn && fn->body.has_value()) return fn;
        }
        return nullptr;
    };
    // Strategy 1: Search from interfaceDef's parent scope (always reliable —
    // interfaces are proper parsed AST nodes with a valid parent chain)
    auto ifaceParent = interfaceDef->parent();
    if (ifaceParent) {
        if (ifaceParent->kind() == ASTNodeKind::NamespaceDecl) {
            auto result = searchNodes(ifaceParent->as_namespace_unsafe()->nodes);
            if (result) return result;
        } else if (ifaceParent->kind() == ASTNodeKind::FileScope) {
            // FileScope has `body.nodes` (Scope), not `nodes` directly
            auto result = searchNodes(static_cast<FileScope*>(ifaceParent)->body.nodes);
            if (result) return result;
        } else if (ifaceParent->kind() == ASTNodeKind::Scope) {
            auto result = searchNodes(static_cast<Scope*>(ifaceParent)->nodes);
            if (result) return result;
        }
    }
    // Strategy 2 (fallback): Search from structDef's parent scope.
    // Less reliable because struct defs from generic instantiations or
    // comptime may have corrupted parent pointers.
    auto structParent = structDef->parent();
    if (structParent && reinterpret_cast<intptr_t>(structParent) > 0x1000) {
        if (structParent->kind() == ASTNodeKind::NamespaceDecl) {
            return searchNodes(structParent->as_namespace_unsafe()->nodes);
        } else if (structParent->kind() == ASTNodeKind::FileScope) {
            return searchNodes(static_cast<FileScope*>(structParent)->body.nodes);
        } else if (structParent->kind() == ASTNodeKind::Scope) {
            return searchNodes(static_cast<Scope*>(structParent)->nodes);
        }
    }
    return nullptr;
}

/**
 * Searches for an interface implementation function for primitive types.
 * Unlike find_interface_impl_fn which matches struct definitions by AST node,
 * this matches by comparing the parent's BaseType with impl->struct_type
 * using is_same(). This enables dispatch for impls like:
 *   impl MyInterface for int { ... }
 *   impl MyInterface for &int { ... }
 *   impl MyInterface for *int { ... }
 */
static FunctionDeclaration* find_primitive_interface_impl_fn(
    InterfaceDefinition* interfaceDef,
    const chem::string_view& funcName,
    BaseType* parentType
) {
    auto searchNodes = [&](const std::vector<ASTNode*>& nodes) -> FunctionDeclaration* {
        for (auto child : nodes) {
            if (!child || child->kind() != ASTNodeKind::ImplDecl) continue;
            auto impl = child->as_impl_def_unsafe();
            if (!impl || !impl->struct_type) continue;
            if (!impl->interface_type) continue;
            auto implInterfaceNode = impl->interface_type->get_direct_linked_node();
            bool interfaceMatches = (implInterfaceNode == interfaceDef);
            if (!interfaceMatches && implInterfaceNode && interfaceDef &&
                implInterfaceNode->get_node_identifier() == interfaceDef->get_node_identifier()) {
                interfaceMatches = true;
            }
            if (!interfaceMatches) continue;
            if (!parentType->is_same(impl->struct_type)) continue;
            auto fn = impl->direct_child_function(funcName);
            if (fn && fn->body.has_value()) return fn;
        }
        return nullptr;
    };
    auto ifaceParent = interfaceDef->parent();
    if (ifaceParent) {
        if (ifaceParent->kind() == ASTNodeKind::NamespaceDecl) {
            return searchNodes(ifaceParent->as_namespace_unsafe()->nodes);
        } else if (ifaceParent->kind() == ASTNodeKind::FileScope) {
            return searchNodes(static_cast<FileScope*>(ifaceParent)->body.nodes);
        } else if (ifaceParent->kind() == ASTNodeKind::Scope) {
            return searchNodes(static_cast<Scope*>(ifaceParent)->nodes);
        }
    }
    return nullptr;
}

/**
 * Handles a function call where the callable has already been evaluated from the
 * parent_val (e.g. when parent_val is itself a FunctionCall that returns a function
 * pointer, an AccessChain to a function, or a LambdaFunction). The 'parent' argument
 * is the already-evaluated callable value.
 */
Value* interpret_value_with_evaluated_parent(FunctionCall* call, InterpretScope &scope, Value* evaluated) {
    if(!evaluated) {
        scope.error("(function call) evaluated callable is null", call);
        return scope.global->typeBuilder.getNullValue();
    }
    // Lambda functions are directly callable
    if(evaluated->kind() == ValueKind::LambdaFunc) {
        auto lambda = static_cast<LambdaFunction*>(evaluated);
        return lambda->call(&scope, scope.allocator, (Value*) nullptr, call->values);
    }
    // An AccessChain/Identifier that links to a FunctionDeclaration — call it directly.
    // This is the case for results of intrinsics::get_child_fn<T>("name") which return
    // an AccessChain whose final Identifier is linked to the target FunctionDeclaration.
    ASTNode* linked = evaluated->linked_node();
    if(linked && linked->kind() == ASTNodeKind::FunctionDecl) {
        auto func_decl = linked->as_function_unsafe();
        if(func_decl) {
            return func_decl->call(&scope, scope.allocator, call, nullptr);
        }
    }
    scope.error("(function call) calling a function that is not found or has no body", call);
    return scope.global->typeBuilder.getNullValue();
}

Value* interpret_value(FunctionCall* call, InterpretScope &scope, Value* parent) {
    auto func = call->safe_linked_func();
    if (func) {
        // If this is a member function and no parent was provided, look for self/this in scope
        if(!parent && func->has_self_param()) {
            auto foundSelf = scope.find_value("self");
            if(foundSelf) {
                parent = foundSelf;
            } else {
                auto foundThis = scope.find_value("this");
                if(foundThis) {
                    parent = foundThis;
                }
            }
        }
        // Interface dispatch: if function has no body and is an interface method,
        // find and call the concrete implementation for the receiver's struct type.
        if(!func->body.has_value() && func->parent() &&
           func->parent()->kind() == ASTNodeKind::InterfaceDecl && parent) {
            auto interfaceDef = func->parent()->as_interface_def_unsafe();
            ExtendableMembersContainerNode* structDef = nullptr;
            // Try to get struct definition from the value directly
            if(parent->val_kind() == ValueKind::StructValue) {
                structDef = parent->as_struct_value_unsafe()->linked_extendable();
                // Fallback: if linked_extendable is null, use type-based resolution
                if(!structDef) {
                    auto parentType = parent->getType();
                    if(parentType) {
                        auto structNode = parentType->get_direct_linked_canonical_node();
                        if(structNode && (structNode->kind() == ASTNodeKind::StructDecl ||
                                          structNode->kind() == ASTNodeKind::VariantDecl)) {
                            structDef = structNode->as_extendable_members_container_unsafe();
                        }
                    }
                }
            } else if(parent->val_kind() == ValueKind::DynamicValue) {
                auto dynVal = static_cast<DynamicValue*>(parent);
                // Strategy 1: try to evaluate the inner value to get the actual struct
                if(dynVal->value) {
                    auto evaluated = dynVal->value->evaluated_value(scope);
                    if(evaluated && evaluated->val_kind() == ValueKind::StructValue) {
                        structDef = evaluated->as_struct_value_unsafe()->linked_extendable();
                    }
                }
                // Strategy 2: fallback to type-based resolution from the inner value's type
                if(!structDef && dynVal->value && dynVal->value->getType()) {
                    auto typeNode = dynVal->value->getType()->get_direct_linked_canonical_node();
                    if(typeNode && (typeNode->kind() == ASTNodeKind::StructDecl ||
                                    typeNode->kind() == ASTNodeKind::VariantDecl)) {
                        structDef = typeNode->as_extendable_members_container_unsafe();
                    }
                }
            } else if(parent->val_kind() == ValueKind::Identifier) {
                // Evaluate the identifier: try scope lookup first, then evaluated_value
                auto identStr = parent->as_identifier_unsafe()->value;
                auto foundVal = scope.find_value(identStr);
                Value* evaluated = foundVal ? foundVal : parent->evaluated_value(scope);
                if(evaluated) {
                    if(evaluated->val_kind() == ValueKind::StructValue) {
                        structDef = evaluated->as_struct_value_unsafe()->linked_extendable();
                    } else if(evaluated->val_kind() == ValueKind::DynamicValue) {
                        auto dynVal = static_cast<DynamicValue*>(evaluated);
                        if(dynVal->value) {
                            auto inner = dynVal->value->evaluated_value(scope);
                            if(inner && inner->val_kind() == ValueKind::StructValue) {
                                structDef = inner->as_struct_value_unsafe()->linked_extendable();
                            }
                        }
                    }
                }
            } else if(parent->val_kind() == ValueKind::PointerValue) {
                // For pointer receivers, get the pointed-to struct type
                auto ptrVal = static_cast<PointerValue*>(parent);
                if(ptrVal && ptrVal->getType()) {
                    auto ptrType = ptrVal->getType();
                    // PointerValue's type is the pointer type; unwrap to find the pointee
                    if(ptrType->kind() == BaseTypeKind::Pointer) {
                        auto pointeeType = static_cast<PointerType*>(ptrType)->type;
                        if(pointeeType) {
                            auto typeNode = pointeeType->get_direct_linked_canonical_node();
                            if(typeNode && (typeNode->kind() == ASTNodeKind::StructDecl ||
                                            typeNode->kind() == ASTNodeKind::VariantDecl)) {
                                structDef = typeNode->as_extendable_members_container_unsafe();
                            }
                        }
                    }
                }
            }
            if(structDef) {
                auto implFunc = find_interface_impl_fn(
                    interfaceDef,
                    structDef,
                    func->name_view()
                );
                if(implFunc) {
                    // Unwrap DynamicValue/Identifier parent to get the underlying
                    // struct value, so that the impl function body can access
                    // fields via self.xxx.
                    Value* effectiveParent = parent;
                    int maxUnwrap = 5;
                    while(effectiveParent && maxUnwrap-- > 0) {
                        if(effectiveParent->val_kind() == ValueKind::DynamicValue) {
                            auto dynVal = static_cast<DynamicValue*>(effectiveParent);
                            if(dynVal->value) {
                                effectiveParent = dynVal->value->evaluated_value(scope);
                                continue;
                            }
                        }
                        if(effectiveParent->val_kind() == ValueKind::Identifier) {
                            auto ident = static_cast<VariableIdentifier*>(effectiveParent);
                            auto found = scope.find_value(ident->value);
                            if(found) {
                                effectiveParent = found;
                                continue;
                            }
                            // If not in scope, try evaluated_value as fallback
                            effectiveParent = effectiveParent->evaluated_value(scope);
                            continue;
                        }
                        break;
                    }
                    auto result = implFunc->call(&scope, scope.allocator, call, effectiveParent);
                    if(result) return result;
                    return scope.global->typeBuilder.getNullValue();
                }
            }
            // Also try looking up the impl from the parent type if the direct lookup failed
            // This handles cases where the parent is a DynamicValue wrapping a struct
            if(!structDef && parent->getType()) {
                // Try name-based resolution: search for the struct type by name in scope
                auto parentType = parent->getType();
                // Walk up to find struct definition through the type's linked chain
                ASTNode* typeNode = parentType->get_direct_linked_node();
                if(typeNode && (typeNode->kind() == ASTNodeKind::StructDecl ||
                                typeNode->kind() == ASTNodeKind::VariantDecl)) {
                    structDef = typeNode->as_extendable_members_container_unsafe();
                    if(structDef) {
                        auto implFunc = find_interface_impl_fn(
                            interfaceDef,
                            structDef,
                            func->name_view()
                        );
                        if(implFunc) {
                            auto result = implFunc->call(&scope, scope.allocator, call, parent);
                            if(result) return result;
                        }
                    }
                }
                // Try primitive type impl matching when all struct-based resolution failed
                if(!structDef) {
                    // For DynamicValue, extract the inner value and its type;
                    // parent->getType() returns DynamicType, not the underlying primitive type.
                    BaseType* primitiveType = parentType;
                    Value* primitiveSelf = parent;
                    if(parent->val_kind() == ValueKind::DynamicValue) {
                        auto dynVal = static_cast<DynamicValue*>(parent);
                        if(dynVal->value) {
                            auto evalInner = dynVal->value->evaluated_value(scope);
                            if(evalInner && evalInner != parent) {
                                primitiveSelf = evalInner;
                                if(evalInner->getType()) {
                                    primitiveType = evalInner->getType();
                                }
                            } else if(dynVal->value->getType()) {
                                primitiveType = dynVal->value->getType();
                            }
                        }
                    }
                    auto implFunc = find_primitive_interface_impl_fn(
                        interfaceDef,
                        func->name_view(),
                        primitiveType
                    );
                    if(implFunc) {
                        Value* selfPtr = primitiveSelf;
                        if(primitiveSelf->val_kind() != ValueKind::PointerValue) {
                            auto byteSize = primitiveType->byte_size(scope.global->target_data);
                            switch(primitiveSelf->val_kind()) {
                                case ValueKind::IntN: {
                                    auto intVal = static_cast<IntNumValue*>(primitiveSelf);
                                    selfPtr = new (scope.allocate<PointerValue>()) PointerValue(
                                        &intVal->value, primitiveType, 0, byteSize, call->encoded_location()
                                    );
                                    break;
                                }
                                case ValueKind::Bool: {
                                    auto boolVal = static_cast<BoolValue*>(primitiveSelf);
                                    selfPtr = new (scope.allocate<PointerValue>()) PointerValue(
                                        &boolVal->value, primitiveType, 0, 1, call->encoded_location()
                                    );
                                    break;
                                }
                                case ValueKind::Float: {
                                    auto floatVal = static_cast<FloatValue*>(primitiveSelf);
                                    selfPtr = new (scope.allocate<PointerValue>()) PointerValue(
                                        &floatVal->value, primitiveType, 0, sizeof(float), call->encoded_location()
                                    );
                                    break;
                                }
                                case ValueKind::Double: {
                                    auto doubleVal = static_cast<DoubleValue*>(primitiveSelf);
                                    selfPtr = new (scope.allocate<PointerValue>()) PointerValue(
                                        &doubleVal->value, primitiveType, 0, sizeof(double), call->encoded_location()
                                    );
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                        if(selfPtr) {
                            auto result = implFunc->call(&scope, scope.allocator, call, selfPtr);
                            if(result) return result;
                        }
                    }
                }
            }
        }
        // Also try interface dispatch when parent is null but we can resolve the type
        // from the function's parent interface and search scope
        if(!func->body.has_value() && !parent &&
           func->parent() && func->parent()->kind() == ASTNodeKind::InterfaceDecl) {
            // Try to find a receiver from scope (self/this or first arg with struct type)
            auto foundSelf = scope.find_value("self");
            if(!foundSelf) foundSelf = scope.find_value("this");
            if(foundSelf && foundSelf->getType()) {
                auto typeNode = foundSelf->getType()->get_direct_linked_canonical_node();
                if(typeNode && (typeNode->kind() == ASTNodeKind::StructDecl ||
                                typeNode->kind() == ASTNodeKind::VariantDecl)) {
                    auto structDef = typeNode->as_extendable_members_container_unsafe();
                    auto interfaceDef = func->parent()->as_interface_def_unsafe();
                    auto implFunc = find_interface_impl_fn(
                        interfaceDef, structDef, func->name_view()
                    );
                    if(implFunc) {
                        auto result = implFunc->call(&scope, scope.allocator, call, foundSelf);
                        if(result) return result;
                    }
                }
            }
        }
        if(!func->body.has_value() && func->parent() && func->parent()->kind() == ASTNodeKind::InterfaceDecl) {
        }
        return func->call(&scope, scope.allocator, call, parent);
    }
    // For direct function calls (parent is nullptr from get_parent_from(Identifier)),
    // try evaluating the parent_val in scope to find lambda values or function pointers
    if(!parent && call->parent_val) {
        auto evaluated = call->parent_val->evaluated_value(scope);
        if(!evaluated) {
            scope.error("(function call) evaluated value is null", call);
            return scope.global->typeBuilder.getNullValue();
        }
        // Use the general evaluator that handles LambdaFunc, Identifier, AccessChain,
        // and any other callable value (e.g. function pointers from get_child_fn)
        return interpret_value_with_evaluated_parent(call, scope, evaluated);
    }
    // When parent is a struct value, look up the member being called and dispatch
    if(parent) {
        chem::string_view memberName;
        if(call->parent_val && call->parent_val->val_kind() == ValueKind::AccessChain) {
            auto chain = call->parent_val->as_access_chain_unsafe();
            if(!chain->values.empty()) {
                auto lastVal = chain->values.back();
                if(lastVal->val_kind() == ValueKind::Identifier) {
                    memberName = lastVal->as_identifier_unsafe()->value;
                }
            }
        }
        if(!memberName.empty()) {
            auto memberVal = parent->child(scope, memberName);
            if(memberVal) {
                return interpret_value_with_evaluated_parent(call, scope, memberVal);
            }
        }
    }
    // Handle variant member constructor calls (e.g., VariantType.MemberName(args))
    {
        chem::string_view memberName;
        ASTNode* variantDef = nullptr;
        if(parent && parent->kind() == ValueKind::Identifier) {
            auto parentId = parent->as_identifier_unsafe();
            if(parentId->linked) {
                auto lk = parentId->linked->kind();
                if(lk == ASTNodeKind::VariantDecl || lk == ASTNodeKind::GenericVariantDecl) {
                    variantDef = parentId->linked;
                    if(call->parent_val->kind() == ValueKind::AccessChain) {
                        auto chain = call->parent_val->as_access_chain_unsafe();
                        auto nameVal = chain->values.back();
                        if(nameVal->kind() == ValueKind::Identifier) {
                            memberName = nameVal->as_identifier_unsafe()->value;
                        }
                    }
                }
            }
        } else if(!parent && call->parent_val) {
            auto evaluated = call->parent_val->evaluated_value(scope);
            if(evaluated && evaluated->kind() == ValueKind::Identifier) {
                auto id = evaluated->as_identifier_unsafe();
                if(id->linked && id->linked->kind() == ASTNodeKind::VariantMember) {
                    variantDef = id->linked->parent();
                    memberName = id->value;
                }
            }
        }
        if(variantDef && !memberName.empty()) {
            auto memberNode = variantDef->child(memberName);
            if(memberNode && memberNode->kind() == ASTNodeKind::VariantMember) {
                auto member = memberNode->as_variant_member_unsafe();
                auto structType = call->getType();                    auto structVal = new (scope.allocate<StructValue>()) StructValue(structType, call->encoded_location());
                // Store the variant member index in the global interpret scope map
                // (not on the StructValue itself, to avoid storing interpretation state in AST nodes)
                // variantDef is ASTNode* and could be VariantDecl or GenericVariantDecl;
                // get the proper VariantDefinition* for direct_child_index
                int64_t memberIndex = -1;
                VariantDefinition* actualVariantDef = nullptr;
                if(variantDef->kind() == ASTNodeKind::VariantDecl) {
                    actualVariantDef = variantDef->as_variant_def_unsafe();
                    memberIndex = actualVariantDef->direct_child_index(member->name);
                } else if(variantDef->kind() == ASTNodeKind::GenericVariantDecl) {
                    actualVariantDef = variantDef->as_gen_variant_decl_unsafe()->master_impl;
                    memberIndex = actualVariantDef->direct_child_index(member->name);
                }
                if(memberIndex >= 0) {
                    structVal->set_variant_member_index(memberIndex);
                }
                // Initialize inherited fields from parent structs
                if(actualVariantDef) {
                    for(auto& inh : actualVariantDef->inherited) {
                        auto linkedNode = inh.type->get_direct_linked_canonical_node();
                        if(linkedNode && linkedNode->kind() == ASTNodeKind::StructDecl) {
                            auto inheritedStruct = linkedNode->as_struct_def_unsafe();
                            for(auto field : inheritedStruct->variables()) {
                                auto defValue = field->default_value();
                                if(defValue) {
                                    auto val = defValue->scope_value(scope);
                                    structVal->values.emplace(field->name, StructMemberInitializer { field->name, val });
                                    if(val) {
                                        scope.move_clear_source(val, field->name);
                                    }
                                }
                            }
                            // Call inherited struct's @make constructor to fill fields
                            // that don't have per-field default values
                            auto parentCons = inheritedStruct->get_first_constructor();
                            if(parentCons) {
                                // Check if any inherited fields are already set
                                bool hasParentMembers = false;
                                for(const auto pvar : inheritedStruct->variables()) {
                                    if(structVal->values.find(pvar->name) != structVal->values.end()) {
                                        hasParentMembers = true;
                                        break;
                                    }
                                }
                                if(!hasParentMembers) {
                                    InterpretScope nested_scope(scope.global, scope.allocator, scope.global);
                                    std::vector<Value*> nested_args;
                                    Value* parentVal = parentCons->call(
                                        &scope, nested_args, (Value*)nullptr, &nested_scope, true, (Value*)nullptr
                                    );
                                    if(parentVal && parentVal->kind() == ValueKind::StructValue) {
                                        auto parentStructVal = parentVal->as_struct_value_unsafe();
                                        for(auto& [pname, pmember] : parentStructVal->values) {
                                            if(structVal->values.find(pname) == structVal->values.end()) {
                                                structVal->values.emplace(pname, pmember);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                for(auto& [paramName, param] : member->values) {
                    auto idx = param->index;
                    Value* paramVal = nullptr;
                    if(idx < call->values.size()) {
                        paramVal = call->values[idx]->evaluated_value(scope);
                    } else if(param->def_value) {
                        // Use default value when no argument is provided
                        paramVal = param->def_value->evaluated_value(scope);
                    }
                    if(paramVal) {
                        // Apply implicit constructor conversion if needed
                        auto paramType = param->type->canonical();
                        auto implicit_constructor = paramType->implicit_constructor_for(paramVal);
                        if(implicit_constructor) {
                            InterpretScope imp_scope(scope.global, scope.allocator, scope.global);
                            std::vector<Value*> imp_args = { paramVal };
                            paramVal = implicit_constructor->call(
                                &scope, imp_args, (Value*)nullptr, &imp_scope, true, (Value*)nullptr
                            );
                        }
                        // Move semantics: if the argument references an existing destructible
                        // struct variable in the caller's scope, clear the source to prevent
                        // double-destruction. Uses pointer-matching instead of AST node type
                        // checks, because the compiler may have replaced identifiers.
                        if(idx < call->values.size()) {
                            scope.move_clear_source(paramVal, chem::string_view());
                        }
                        structVal->values.insert({paramName, StructMemberInitializer(paramName, paramVal)});
                    }
                }
                return structVal;
            }
        }
    }
    scope.error("(function call) calling a function that is not found or has no body", call);
    return scope.global->typeBuilder.getNullValue();
}

Value* FunctionCall::evaluated_value(InterpretScope &scope) {
    // When the parent_val is itself a callable expression (e.g. a FunctionCall like
    // 'give_me_some_sum()' in 'give_me_some_sum()(9, 4)', or an AccessChain that resolves
    // to a function pointer), we must evaluate it first to obtain the callable value,
    // rather than traversing into its inner parent_val via get_parent_from.
    if(parent_val && parent_val->val_kind() == ValueKind::FunctionCall) {
        auto evaluated = parent_val->evaluated_value(scope);
        auto result = interpret_value_with_evaluated_parent(this, scope, evaluated);
        if(evaluated && evaluated != result) {
            scope.destroy_value(evaluated);
        }
        return result;
    }
    const auto parent = get_parent_from(parent_val);
    Value* evaluated_parent;
    if(parent && parent_val && parent_val->val_kind() == ValueKind::AccessChain) {
        // When the parent_val is an AccessChain (e.g., c.p in c.p.call()),
        // evaluate the chain's prefix to get the actual receiver value,
        // rather than just the penultimate element (which may be an Identifier
        // that doesn't evaluate on its own).
        auto chain = parent_val->as_access_chain_unsafe();
        if(chain->values.size() >= 2) {
            auto first = chain->values[0]->evaluated_value(scope);
            // Evaluate up to the second-to-last element (skip the last AccessChain element)
            unsigned endIdx = (unsigned)(chain->values.size() - 1);
            evaluated_parent = first;
            for(unsigned ci = 1; ci < endIdx; ci++) {
                auto val = chain->values[ci];
                if(val->val_kind() == ValueKind::Identifier) {
                    auto id = val->as_identifier_unsafe();
                    evaluated_parent = evaluated_parent ? evaluated_parent->child(scope, id->value) : nullptr;
                } else {
                    evaluated_parent = val->evaluated_value(scope);
                }
                if(!evaluated_parent) break;
            }
        } else {
            evaluated_parent = parent->evaluated_value(scope);
        }
    } else {
        evaluated_parent = parent ? parent->evaluated_value(scope) : parent;
    }
    auto result = interpret_value(this, scope, evaluated_parent);
    // If the parent was itself a FunctionCall (temp from nested method call),
    // destruct the intermediate temp after using it as the receiver.
    if(parent && parent->val_kind() == ValueKind::FunctionCall && evaluated_parent && evaluated_parent != result) {
        scope.destroy_value(evaluated_parent);
    }
    return result;
}

FunctionCall *FunctionCall::copy(ASTAllocator& allocator) {
    auto call = new (allocator.allocate<FunctionCall>()) FunctionCall((Value*) parent_val->copy(allocator), getType(), encoded_location());
    for(const auto value : values) {
        call->values.emplace_back(value->copy(allocator));
    }
    for(auto& gen_arg : generic_list) {
        call->generic_list.emplace_back(gen_arg.copy(allocator));
    }
    return call;
}

VoidType* voidTy(ASTAllocator& allocator) {
    return new (allocator.allocate<VoidType>()) VoidType();
}

FunctionType* get_func_type(BaseType* parent_type, std::vector<Value*>& args) {
    const auto can_type = parent_type->canonical();
    if(can_type->kind() == BaseTypeKind::CapturingFunction) {
        return can_type->as_capturing_func_type_unsafe()->func_type->as_function_type();
    }
    if(can_type->kind() == BaseTypeKind::Reference) {
        const auto refType = can_type->as_reference_type_unsafe()->type->canonical();
        if(refType->kind() == BaseTypeKind::CapturingFunction) {
            return refType->as_capturing_func_type_unsafe()->func_type->as_function_type();
        }
    }
    if(can_type->kind() == BaseTypeKind::Linked || can_type->kind() == BaseTypeKind::Generic) {
        const auto container = can_type->get_direct_linked_canonical_node();
        if(container) {
            switch(container->kind()) {
                case ASTNodeKind::StructDecl:{
                    // linked with master implementation
                    return container->as_struct_def_unsafe()->constructor_func(args);
                }
                case ASTNodeKind::VariantDecl: {
                    // linked with master implementation
                    return container->as_variant_def_unsafe()->constructor_func(args);
                }
                default:
                    break;
            }
        }
    }
    return can_type->as_function_type();
}

void FunctionCall::determine_type(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    const auto call = this;
    const auto newLinked = parent_val->linked_node();
    // enum member being used as a no value
    const auto newLinkedKind = newLinked ? newLinked->kind() : ASTNodeKind::EnumMember;
    if(newLinkedKind == ASTNodeKind::FunctionDecl) {
        const auto func = newLinked->as_function_unsafe();
        if (func->is_constructor_fn() && func->parent()) {
            // we do not support constructors in variants
            const auto parent = func->parent();
            if(parent->kind() == ASTNodeKind::StructDecl || parent->kind() == ASTNodeKind::VariantDecl) {
                const auto container = parent->as_members_container_unsafe();
                if (container && container->generic_parent != nullptr) {
                    call->setType(new(allocator.allocate<GenericType>()) GenericType(new(allocator.allocate<LinkedType>()) LinkedType(container), call->generic_list));
                } else {
                    call->setType(new(allocator.allocate<LinkedType>()) LinkedType(container));
                }
            } else {
                call->setType(new(allocator.allocate<LinkedType>()) LinkedType(parent));
            }
        } else {
            call->setType(func->returnType);
        }
    } else if(newLinkedKind == ASTNodeKind::VariantMember) {
        // this must be non generic variant member call
        // generics are handle in instantiate_block
        call->setType(newLinked->as_variant_member_unsafe()->known_type());
    } else {
        const auto func_type = get_func_type(call->parent_val->getType(), call->values);
        if(func_type) {
            call->setType(func_type->returnType);
        } else {
            diagnoser.error(call) << "couldn't determine function type for the given call";
        }
    }
}