// Copyright (c) Qinetik 2024.

#include "ASTUtils.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/GenericType.h"
#include "GenericUtils.h"
#include "compiler/SymbolResolver.h"

bool chain_contains_func_call(std::vector<std::unique_ptr<ChainValue>>& values, int start, int end) {
    while(start < end) {
        if(values[start]->as_func_call()) {
            return true;
        }
        start++;
    }
    return false;
}

void evaluate_values(std::vector<std::unique_ptr<Value>>& values, InterpretScope& scope) {
    for(auto& value : values) {
        auto evaluated = value->evaluated_value(scope);
        if(evaluated.get() == value.get()) continue;
        if(evaluated.get_will_free()) {
            value.reset(evaluated.release());
        } else {
            value.reset(evaluated->copy());
        }
    }
}

std::unique_ptr<Value> call_with_arg(FunctionDeclaration* decl, std::unique_ptr<Value> arg, SymbolResolver& resolver) {
    auto chain = std::make_unique<AccessChain>(nullptr, false, nullptr);
    auto id = std::make_unique<VariableIdentifier>(decl->name, nullptr);
    id->linked = decl;
    chain->values.emplace_back(std::move(id));
    auto imp_call = std::make_unique<FunctionCall>(std::vector<std::unique_ptr<Value>> {}, nullptr);
    imp_call->parent_val = chain->values[0].get();
    imp_call->values.emplace_back(std::move(arg));
    auto& value_ptr = imp_call->values[0];
    value_ptr->link(resolver, value_ptr, imp_call->get_arg_type(0));
    chain->values.emplace_back(std::move(imp_call));
    return chain;
}

void link_with_implicit_constructor(FunctionDeclaration* decl, SymbolResolver& resolver, Value* value) {
    VariableIdentifier id(decl->name, nullptr);
    id.linked = decl;
    FunctionCall imp_call(std::vector<std::unique_ptr<Value>>{}, nullptr);
    imp_call.parent_val = &id;
    imp_call.values.emplace_back(value);
    imp_call.find_link_in_parent(&id, resolver, nullptr, false);
    const auto replaced = imp_call.values[0].release();
#ifdef DEBUG
    if(replaced != value) {
        throw std::runtime_error("implicit constructor value has been replaced, when it shouldn't have been");
    }
#endif
}

int16_t get_iteration_for(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params, std::vector<BaseType*>& generic_list) {
    if(!generic_params.empty()) {
        int16_t i = 0;
        unsigned j;
        const auto total = generic_params[0]->usage.size();
        while(i < total) {
            j = 0;
            bool all_params_found = true;
            for(auto& param : generic_params) {
                const auto generic_arg = j < generic_list.size() ? generic_list[j] : nullptr;
                const auto generic_arg_pure = generic_arg ? generic_arg : param->def_type.get();
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

int16_t total_generic_iterations(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params) {
    if(generic_params.empty()) {
        return 1;
    } else {
        return (int16_t) generic_params[0]->usage.size();
    }
}

int16_t register_generic_usage_no_check(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params, std::vector<BaseType*>& generic_list) {
    int16_t i = 0;
    for (auto &param: generic_params) {
        param->register_usage(i < generic_list.size() ? generic_list[i] : nullptr);
        i++;
    }
    return (int16_t) ((int16_t) total_generic_iterations(generic_params) - (int16_t) 1);
}

std::pair<int16_t, bool> register_generic_usage(
        SymbolResolver& resolver,
        ASTNode* node,
        std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params,
        std::vector<BaseType*>& generic_list
) {
    int16_t i = get_iteration_for(generic_params, generic_list);
    if(i != -1) return { i, false};
    resolver.imported_generic[node] = true;
    return { register_generic_usage_no_check(generic_params, generic_list), true };
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
        // directly linked generic param like func <T> add(param : T)
        const auto linked = param_type->linked_node();
        const auto gen_param = linked->as_generic_type_param();
        if(gen_param && gen_param->parent_node == params_node && gen_param->param_index >= generic_list_size && !gen_param->def_type) {
            // get the function argument for this arg_offset
            inferred[gen_param->param_index] = arg_type;
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
                    infer_types_by_args(diagnoser, params_node, generic_list_size, param_type_gen->types[i].get(), arg_type_gen->types[i].get(), inferred, debug_value);
                    i++;
                }
            } else {
                diagnoser.error("given types generic arguments don't have equal length, for " + param_type_gen->representation() + ", given " + arg_type_gen->representation(), debug_value);
            }
        }
    }
}