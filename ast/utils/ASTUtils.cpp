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

bool has_function_call_before(Value* value) {
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

VariableIdentifier* get_first_chain_id(Value* value) {
    switch(value->val_kind()) {
        case ValueKind::Identifier:
            return value->as_identifier_unsafe();
        case ValueKind::AccessChain:{
            const auto chain = value->as_access_chain_unsafe();
            return get_first_chain_value(chain->values.front());
        }
        case ValueKind::FunctionCall:
            return get_first_chain_value(value->as_func_call_unsafe()->parent_val);
        case ValueKind::IndexOperator:
            return get_first_chain_value(value->as_index_op_unsafe()->parent_val);
        default:
            return nullptr;
    }
}

Value* get_parent_from(Value* value) {
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

Value* get_grandpa_from(Value* value) {
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

Value* build_parent_chain(std::vector<Value*>& values, ASTAllocator& allocator) {
    if(values.size() > 1) {
        if(values.size() == 2) {
            return values.front();
        } else {
            const auto first = values.front();
            const auto parent = new (allocator.allocate<AccessChain>()) AccessChain(first->encoded_location());
            unsigned i = 0;
            while(i < values.size() - 1) {
                parent->values.emplace_back(values[i]);
                i++;
            }
            parent->setType(parent->values.back()->getType());
            return parent;
        }
    } else {
        return nullptr;
    }
}

Value* build_parent_chain(Value* value, ASTAllocator& allocator) {
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
    id->setType(decl->known_type());
    auto imp_call = new (allocator.allocate<FunctionCall>()) FunctionCall(id, location);
    imp_call->parent_val = id;
    imp_call->values.emplace_back(arg);
    imp_call->setType(decl->returnType);
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

    // infer args, if user gave less args than expected
    if(call->generic_list.size() != total) {
        call->infer_generic_args(allocator, diagnoser, out_generic_args);
    }

    if(expected_type) {
        call->infer_return_type(diagnoser, out_generic_args, expected_type);
    }

}

int16_t get_iteration_for(
        const std::span<std::span<BaseType*>>& instantiations,
        std::vector<TypeLoc>& generic_list
) {
    if(instantiations.empty()) return -1;
    int16_t i = 0;
    unsigned j;
    const auto total = instantiations.size();
    while(i < total) {
        bool all_params_found = true;
        auto& instantiation = instantiations[i];
        j = 0;
        for(const auto instType : instantiation) {
            const auto generic_arg_pure = generic_list[j];
            if(!generic_arg_pure || !instType->canonical()->is_same(generic_arg_pure->canonical())) {
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
        *vec = const_cast<BaseType*>(type.getType())->copy(astAllocator);
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

TypeLoc get_param_def_type(BaseType* type) {
    const auto linked = type->get_direct_linked_node();
    if(linked && linked->kind() == ASTNodeKind::GenericTypeParam) {
        return linked->as_generic_type_param_unsafe()->def_type;
    } else {
        return nullptr;
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
                auto& inferred_type = inferred[gen_param->param_index];
                if(inferred_type == nullptr && is_node_parent_of(params_node, gen_param) && gen_param->param_index >= generic_list_size) {
                    // get the function argument for this arg_offset
                    inferred_type = arg_type;
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
                unsigned i = 0;
                while(i < child_gen_size) {
                    const auto param_type_child = param_type_gen->types[i];
                    const auto arg_type_child = i < arg_type_gen->types.size() ? arg_type_gen->types[i] : get_param_def_type(param_type_child);
                    if(arg_type_child) {
                        infer_types_by_args(diagnoser, params_node, generic_list_size, param_type_child, arg_type_child, inferred, debug_value);
                    } else {
                        diagnoser.error(debug_value) << "couldn't infer generic argument for " << param_type_gen->representation() << ", at index " << std::to_string(i);
                    }
                    i++;
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

bool initialize_generic_args(
        ASTDiagnoser& diagnoser,
        std::vector<TypeLoc>& out_generic_args,
        std::vector<GenericTypeParameter*>& generic_params,
        std::vector<TypeLoc>& user_generic_list
) {
    // reserve the generic args
    const auto total = generic_params.size();

    // check if user gave way too many generic arguments
    const auto user_given = user_generic_list.size();
    if(user_given > total) {
        diagnoser.error(user_generic_list[total].encoded_location()) << "too many generic arguments given, expected " << std::to_string(total) << " given " << std::to_string(user_given);
        return false;
    }

    // set all to nullptr
    out_generic_args.reserve(total);
    unsigned i = 0;
    while(i < total) {
        out_generic_args.emplace_back(nullptr);
        i++;
    }

    // set given generic args to generic parameters
    i = 0;
    while(i < user_given) {
        out_generic_args[i] = user_generic_list[i];
        i++;
    }

    return true;
}

bool check_inferred_generic_args(
        ASTDiagnoser& diagnoser,
        std::vector<TypeLoc>& generic_args,
        std::vector<GenericTypeParameter*>& generic_params,
        SourceLocation location
) {
    unsigned i = 0;
    const auto total = generic_args.size();
    while(i < total) {
        auto& typeLoc = generic_args[i];
        if(typeLoc == nullptr) {
            auto param = generic_params[i];
            if(param->def_type != nullptr) {
                typeLoc = param->def_type;
                i++;
                continue;
            }
            diagnoser.error(location) << "couldn't infer type for generic parameter at index " << std::to_string(i) << ", no default type has been given";
            return false;
        }
        i++;
    }
    return true;
}