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

/**
 * Handles a call whose callable came from evaluating `parent_val` (e.g. the
 * result of `intrinsics::get_child_fn<T>("name")`, an AccessChain/Identifier
 * linked to a FunctionDeclaration).
 */
Value* interpret_value_with_evaluated_parent(FunctionCall* call, InterpretScope &scope, Value* evaluated) {
    if(!evaluated) {
        scope.error("(function call) evaluated callable is null", call);
        return scope.global->typeBuilder.getNullValue();
    }
    ASTNode* linked = evaluated->linked_node();
    if(linked && linked->kind() == ASTNodeKind::FunctionDecl) {
        auto func_decl = linked->as_function_unsafe();
        if(func_decl) {
            return func_decl->call(&scope, scope.allocator, call, nullptr);
        }
    }
    scope.error("(function call) evaluated callable is not callable at symbol resolution", call);
    return scope.global->typeBuilder.getNullValue();
}

Value* interpret_value(FunctionCall* call, InterpretScope &scope, Value* parent) {
    auto func = call->safe_linked_func();
    if(!func) {
        scope.error("(function call) calling a function that is not found or has no body", call);
        return scope.global->typeBuilder.getNullValue();
    }
    if(!parent && func->has_self_param()) {
        // a member call with no explicit receiver: look for self/this in scope
        if(Value* foundSelf = scope.find_value("self")) {
            parent = foundSelf;
        } else if(Value* foundThis = scope.find_value("this")) {
            parent = foundThis;
        }
    }
    // A compiler intrinsic carries its own `call` override. A declaration with a
    // body folds only when that body is a single `return <expr>`; a body with
    // statements is evaluated by the MIR engine instead.
    if(func->body.has_value() || func->is_compiler_decl()) {
        return func->call(&scope, scope.allocator, call, parent);
    }
    scope.error("(function call) calling a function that is not found or has no body", call);
    return scope.global->typeBuilder.getNullValue();
}

Value* FunctionCall::evaluated_value(InterpretScope &scope) {
    // a callable produced by another call, e.g. `intrinsics::get_child_fn<T>(...)`
    if(parent_val && parent_val->val_kind() == ValueKind::FunctionCall) {
        Value* evaluated = parent_val->evaluated_value(scope);
        return interpret_value_with_evaluated_parent(this, scope, evaluated);
    }
    return interpret_value(this, scope, get_parent_from(parent_val));
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