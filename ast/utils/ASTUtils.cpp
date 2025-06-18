// Copyright (c) Chemical Language Foundation 2025.

#include "ASTUtils.h"
#include "ast/values/AccessChain.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/GenericType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "GenericUtils.h"
#include "compiler/SymbolResolver.h"

bool are_all_specialized(const std::span<BaseType*>& types) {
    for(const auto ty : types) {
        if(!ty || ty->kind() == BaseTypeKind::Linked && ty->as_linked_type_unsafe()->linked->kind() == ASTNodeKind::GenericTypeParam) {
            return false;
        }
    }
    return true;
}

bool are_all_specialized(const std::span<TypeLoc>& types) {
    for(auto& ty : types) {
        if(!ty || ty->kind() == BaseTypeKind::Linked && ty->as_linked_type_unsafe()->linked->kind() == ASTNodeKind::GenericTypeParam) {
            return false;
        }
    }
    return true;
}

bool has_function_call_before(ChainValue* value) {
    switch(value->val_kind()) {
        case ValueKind::Identifier:
            return false;
        case ValueKind::FunctionCall:
            return true;
        case ValueKind::IndexOperator:
            return has_function_call_before(value->as_index_op_unsafe()->parent_val);
        case ValueKind::AccessChain:
            for(const auto child : value->as_access_chain_unsafe()->values) {
                if(has_function_call_before(child)) {
                    return true;
                }
            }
            return false;
        default:
            return false;
    }
}

ChainValue* get_parent_from(ChainValue* value) {
    switch(value->val_kind()) {
        case ValueKind::Identifier:
            return nullptr;
        case ValueKind::AccessChain:{
            const auto chain = value->as_access_chain_unsafe();
            if(chain->values.size() > 1) {
                return chain->values[chain->values.size() - 2];
            } else {
                return nullptr;
            }
        }
        case ValueKind::FunctionCall:
            return value->as_func_call_unsafe()->parent_val;
        case ValueKind::IndexOperator:
            return value->as_index_op_unsafe()->parent_val;
        default:
            return nullptr;
    }
}

ChainValue* get_grandpa_from(ChainValue* value) {
    switch(value->val_kind()) {
        case ValueKind::Identifier:
            return nullptr;
        case ValueKind::AccessChain:{
            const auto chain = value->as_access_chain_unsafe();
            if(chain->values.size() > 2) {
                return chain->values[chain->values.size() - 3];
            } else {
                return nullptr;
            }
        }
        case ValueKind::FunctionCall:
            return get_parent_from(value->as_func_call_unsafe()->parent_val);
        case ValueKind::IndexOperator:
            return get_parent_from(value->as_index_op_unsafe()->parent_val);
        default:
            return nullptr;
    }
}

ChainValue* build_parent_chain(std::vector<ChainValue*>& values, ASTAllocator& allocator) {
    if(values.size() > 1) {
        if(values.size() == 2) {
            return values.front();
        } else {
            const auto first = values.front();
            const auto parent = new (allocator.allocate<AccessChain>()) AccessChain(false, first->encoded_location());
            unsigned i = 0;
            while(i < values.size() - 1) {
                parent->values.emplace_back(values[i]);
                i++;
            }
            return parent;
        }
    } else {
        return nullptr;
    }
}

ChainValue* build_parent_chain(ChainValue* value, ASTAllocator& allocator) {
    switch(value->val_kind()) {
        case ValueKind::Identifier:
            return nullptr;
        case ValueKind::FunctionCall:
            return value->as_func_call_unsafe()->parent_val;
        case ValueKind::IndexOperator:
            return value->as_index_op_unsafe()->parent_val;
        case ValueKind::AccessChain:{
            const auto chain = value->as_access_chain_unsafe();
            return build_parent_chain(chain->values, allocator);
        }
        default:
            return nullptr;
    }
}

void evaluate_values(std::vector<Value*>& values, InterpretScope& scope) {
    for(auto& value : values) {
        auto evaluated = value->evaluated_value(scope);
        if(evaluated == value) continue;
        value = evaluated;
    }
}

FunctionCall* call_with_arg(FunctionDeclaration* decl, Value* arg, BaseType* expected_type, ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    const auto location = arg->encoded_location();
    auto& id_view = decl->identifier.identifier;
    auto str = allocator.allocate_str(id_view.data(), id_view.size());
    auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(chem::string_view(str, id_view.size()), location);
    id->linked = decl;
    auto imp_call = new (allocator.allocate<FunctionCall>()) FunctionCall(id, location);
    imp_call->parent_val = id;
    imp_call->values.emplace_back(arg);
    return imp_call;
}

void infer_generic_args(
    ASTAllocator& allocator,
    std::vector<TypeLoc>& out_generic_args,
    std::vector<GenericTypeParameter*>& generic_params,
    FunctionCall* call,
    ASTDiagnoser& diagnoser,
    BaseType* expected_type
) {

    const auto total = generic_params.size();

    // set all to default type (if default type is not present, it would automatically be nullptr)
    unsigned i = 0;
    while(i < total) {
        out_generic_args[i] = generic_params[i]->def_type;
        i++;
    }

    // set given generic args to generic parameters
    i = 0;
    for(const auto arg : call->generic_list) {
        out_generic_args[i] = arg;
        i++;
    }

    // infer args, if user gave less args than expected
    if(call->generic_list.size() != total) {
        call->infer_generic_args(allocator, diagnoser, out_generic_args);
    }
    if(expected_type) {
        call->infer_return_type(diagnoser, out_generic_args, expected_type);
    }

    // check all types have been inferred
    i = 0;
    for(const auto arg : out_generic_args) {
        if(arg) {
//            if(arg->kind() == BaseTypeKind::Linked && arg->as_linked_type_unsafe()->linked->kind() == ASTNodeKind::GenericTypeParam) {
//                diagnoser.error(call) << "couldn't infer type for generic parameter at index " << std::to_string(i);
//            }
        } else {
            diagnoser.error(call) << "couldn't infer type for generic parameter at index " << std::to_string(i);
        }
        i++;
    }

}

void link_with_implicit_constructor(FunctionDeclaration* decl, SymbolResolver& resolver, Value* value) {
    VariableIdentifier id(decl->name_view(), ZERO_LOC);
    id.linked = decl;
    FunctionCall imp_call(&id, ZERO_LOC);
    imp_call.values.emplace_back(value);
    imp_call.link_without_parent(resolver, nullptr, false);
    const auto replaced = imp_call.values[0];
#ifdef DEBUG
    if(replaced != value) {
        throw std::runtime_error("implicit constructor value has been replaced, when it shouldn't have been");
    }
#endif
}

int16_t get_iteration_for(
        const std::span<std::span<BaseType*>>& instantiations,
        std::vector<TypeLoc>& generic_list
) {
    int16_t i = 0;
    unsigned j;
    const auto total = instantiations.size();
    while(i < total) {
        bool all_params_found = true;
        auto& instantiation = instantiations[i];
        j = 0;
        for(const auto instType : instantiation) {
            const auto generic_arg_pure = generic_list[j]->canonical();
            if(!generic_arg_pure || !instType->is_same(generic_arg_pure)) {
                all_params_found = false;
                break;
            }
            j++;
        }
        if(all_params_found) {
            break;
        }
        i++;
    }
    if(i == total) {
        return -1;
    }
    return i;
}

std::pair<int16_t, bool> register_generic_usage(
        ASTAllocator& astAllocator,
        void* key,
        InstantiationsContainer& container,
        std::vector<TypeLoc>& generic_list,
        std::vector<void*>& instVec
) {

    // check if previous instantiation already exists
    auto instantiations = container.getInstantiationTypesFor(key);
    auto i = get_iteration_for(instantiations, generic_list);
    if(i != -1) return { i, false};

    // allocate generic list vector on the allocator (so we can pass it around as a span)
    const auto initial = (BaseType**) astAllocator.allocate_released_size(sizeof(void*) * generic_list.size(), alignof(void*));
    auto vec = initial;
    for(auto& type : generic_list) {
        *vec = const_cast<BaseType*>(type.getType());
        vec++;
    }
    auto generic_list_allocated = std::span<BaseType*>(initial, generic_list.size());

    // register the instantiation
    const auto index = container.registerInstantiation(key, generic_list_allocated, instVec);
    return { (int16_t) index, true };

}

bool is_node_parent_of(ASTNode* params_node, GenericTypeParameter* param) {
    const auto parent = param->parent();
    if(parent == params_node) {
        return true;
    } else {
        switch(parent->kind()) {
            case ASTNodeKind::FunctionDecl:
                return parent->as_function_unsafe()->generic_parent == params_node;
            default:
                return false;
        }
    }
}

bool are_linked_same(ASTNode* param_node, ASTNode* arg_node) {
    if(param_node == arg_node) {
        return true;
    } else {
        switch(arg_node->kind()) {
            case ASTNodeKind::StructDecl:
                return ((ASTNode*) arg_node->as_struct_def_unsafe()->generic_parent) == param_node;
            case ASTNodeKind::VariantDecl:
                return ((ASTNode*) arg_node->as_variant_def_unsafe()->generic_parent) == param_node;
            default:
                return false;
        }
    }
}

void infer_types_by_args(
        ASTDiagnoser& diagnoser,
        ASTNode* params_node,
        unsigned int generic_list_size,
        BaseType* param_type,
        TypeLoc arg_type,
        std::vector<TypeLoc>& inferred,
        Value* debug_value
) {
    switch(param_type->kind()) {
        case BaseTypeKind::Linked:{
            const auto linked = param_type->linked_node();
            const auto linked_kind = linked->kind();
            if(linked_kind == ASTNodeKind::GenericTypeParam) {
                // directly linked generic param like func <T> add(param : T) or func add() : T
                // so we have param type T which is linked with a generic type parameter and arg type the inferred type
                const auto gen_param = linked->as_generic_type_param_unsafe();
                if(is_node_parent_of(params_node, gen_param) && gen_param->param_index >= generic_list_size) {
                    // get the function argument for this arg_offset
                    inferred[gen_param->param_index] = arg_type;
                }
            } else if(linked_kind == ASTNodeKind::StructDecl) {
                // the arg type is a generic type like in a function return func copy() : MyVector<int> { return { 2 } }
                // however the param type is MyVector<T> and we must infer the types using the arg type
                const auto container = linked->as_members_container_unsafe();
                if(arg_type->kind() == BaseTypeKind::Generic) {
                    // TODO handle this case
                }
            }
            break;
        }
        case BaseTypeKind::Generic: {
            // not directly linked generic param like func <T> add(param : Thing<T>)
            const auto arg_type_gen = (GenericType*) arg_type.getType();
            const auto param_type_gen = (GenericType*) param_type;
            const auto arg_type_kind = arg_type->kind();
            if(arg_type_kind == BaseTypeKind::Generic && are_linked_same(param_type->get_direct_linked_canonical_node(), arg_type->get_direct_linked_canonical_node())) {
                const auto child_gen_size = param_type_gen->types.size();
                if(arg_type_gen->types.size() == child_gen_size) {
                    unsigned i = 0;
                    while(i < child_gen_size) {
                        infer_types_by_args(diagnoser, params_node, generic_list_size, param_type_gen->types[i], arg_type_gen->types[i], inferred, debug_value);
                        i++;
                    }
                } else {
                    diagnoser.error(debug_value) << "given types generic arguments don't have equal length, for " << param_type_gen->representation() << ", given " << arg_type_gen->representation();
                }
            }
            break;
        }
        case BaseTypeKind::Pointer: {
            if(arg_type->kind() == BaseTypeKind::Pointer) {
                infer_types_by_args(diagnoser, params_node, generic_list_size, param_type->as_pointer_type_unsafe()->type, {arg_type->as_pointer_type_unsafe()->type, arg_type.encoded_location()}, inferred, debug_value);
            }
            break;
        }
        case BaseTypeKind::Reference: {
            if(arg_type->kind() == BaseTypeKind::Reference) {
                infer_types_by_args(diagnoser, params_node, generic_list_size, param_type->as_reference_type_unsafe()->type, {arg_type->as_reference_type_unsafe()->type, arg_type.encoded_location()}, inferred, debug_value);
            } else {
                infer_types_by_args(diagnoser, params_node, generic_list_size, param_type->as_reference_type_unsafe()->type, arg_type, inferred, debug_value);
            }
            break;
        }
        default:
            return;
    }
}