// Copyright (c) Chemical Language Foundation 2025.

#include "ASTUtils.h"
#include "ast/values/AccessChain.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/GenericType.h"
#include "GenericUtils.h"
#include "compiler/SymbolResolver.h"

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
    imp_call->fix_generic_iteration(diagnoser, expected_type);
    return imp_call;
}

void infer_generic_args(
    std::vector<BaseType*>& out_generic_args,
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
        call->infer_generic_args(diagnoser, out_generic_args);
    }
    if(expected_type) {
        call->infer_return_type(diagnoser, out_generic_args, expected_type);
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

int16_t get_iteration_for(std::vector<GenericTypeParameter*>& generic_params, std::vector<BaseType*>& generic_list) {
    if(!generic_params.empty()) {
        int16_t i = 0;
        unsigned j;
        const auto total = generic_params[0]->usage.size();
        while(i < total) {
            j = 0;
            bool all_params_found = true;
            for(auto& param : generic_params) {
                const auto generic_arg = j < generic_list.size() ? generic_list[j] : nullptr;
                const auto generic_arg_pure = generic_arg ? generic_arg->pure_type() : param->def_type;
                if(!generic_arg_pure || !param->usage[i]->is_same(generic_arg_pure)) {
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
    } else {
        return 0;
    }
}

int16_t total_generic_iterations(std::vector<GenericTypeParameter*>& generic_params) {
    if(generic_params.empty()) {
        return 1;
    } else {
        return (int16_t) generic_params[0]->usage.size();
    }
}

int16_t register_generic_usage_no_check(
    ASTAllocator& allocator,
    std::vector<GenericTypeParameter*>& generic_params,
    std::vector<BaseType*>& generic_list
) {
    int16_t i = 0;
    for (const auto param: generic_params) {
        const auto arg_type = i < generic_list.size() ? generic_list[i] : param->def_type;
        param->register_usage(allocator, arg_type);
        i++;
    }
    return (int16_t) ((int16_t) total_generic_iterations(generic_params) - (int16_t) 1);
}

std::pair<int16_t, bool> register_generic_usage(
        ASTAllocator& astAllocator,
        std::vector<GenericTypeParameter*>& generic_params,
        std::vector<BaseType*>& generic_list
) {
    int16_t i = get_iteration_for(generic_params, generic_list);
    if(i != -1) return { i, false};
    return {register_generic_usage_no_check(astAllocator, generic_params, generic_list), true };
}

void infer_types_by_args(
        ASTDiagnoser& diagnoser,
        ASTNode* params_node,
        unsigned int generic_list_size,
        BaseType* param_type,
        BaseType* arg_type,
        std::vector<BaseType*>& inferred,
        Value* debug_value
) {
    const auto param_type_kind = param_type->kind();
    if(param_type_kind == BaseTypeKind::Linked) {
         const auto linked = param_type->linked_node();
        const auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::GenericTypeParam) {
            // directly linked generic param like func <T> add(param : T) or func add() : T
            // so we have param type T which is linked with a generic type parameter and arg type the inferred type
            const auto gen_param = linked->as_generic_type_param_unsafe();
            if(gen_param->parent() == params_node && gen_param->param_index >= generic_list_size) {
                // get the function argument for this arg_offset
                inferred[gen_param->param_index] = arg_type;
            }
        } else if(linked_kind == ASTNodeKind::StructDecl) {
            // the arg type is a generic type like in a function return func copy() : MyVector<int> { return { 2 } }
            // however the param type is MyVector<T> and we must infer the types using the arg type
            const auto container = linked->as_members_container_unsafe();
            if(arg_type->kind() == BaseTypeKind::Generic) {
                const auto gen_type = (GenericType*) arg_type;
                unsigned i = 0;
                const auto size = gen_type->types.size();
                while(i < size) {
                    const auto child_arg_type = gen_type->types[i];
                    if(i < container->generic_params.size()) {
                        const auto child_gen_param = container->generic_params[i];
                        inferred[child_gen_param->param_index] = child_arg_type;
                    } else {
                        diagnoser.error("type has been given for a unknown generic type parameter", child_arg_type);
                    }
                    i++;
                }
            }
        }
    } else if(param_type_kind == BaseTypeKind::Generic) {
        // not directly linked generic param like func <T> add(param : Thing<T>)
        const auto arg_type_gen = (GenericType*) arg_type;
        const auto param_type_gen = (GenericType*) param_type;
        if((arg_type->kind() == BaseTypeKind::Generic) && arg_type->linked_struct_def() == param_type->linked_struct_def()) {
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
    }
}