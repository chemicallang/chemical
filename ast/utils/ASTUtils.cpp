// Copyright (c) Qinetik 2024.

#include "ASTUtils.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/FunctionCall.h"
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
    auto chain = std::make_unique<AccessChain>(nullptr, false);
    auto id = std::make_unique<VariableIdentifier>(decl->name);
    id->linked = decl;
    chain->values.emplace_back(std::move(id));
    auto imp_call = std::make_unique<FunctionCall>(std::vector<std::unique_ptr<Value>> {});
    imp_call->parent_val = chain->values[0].get();
    imp_call->values.emplace_back(std::move(arg));
    imp_call->values[0]->link(resolver, imp_call.get(), 0);
    chain->values.emplace_back(std::move(imp_call));
    return chain;
}

int16_t get_iteration_for(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params, std::vector<std::unique_ptr<BaseType>>& generic_list) {
    if(!generic_params.empty()) {
        int16_t i = 0;
        unsigned j;
        const auto total = generic_params[0]->usage.size();
        while(i < total) {
            j = 0;
            bool all_params_found = true;
            for(auto& param : generic_params) {
                const auto generic_arg = j < generic_list.size() ? generic_list[j].get() : nullptr;
                const auto generic_arg_pure = generic_arg ? generic_arg : param->def_type.get();
                if(!param->usage[i]->is_same(generic_arg_pure)) {
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

int16_t register_generic_usage_no_check(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params, std::vector<std::unique_ptr<BaseType>>& generic_list) {
    int16_t i = 0;
    for (auto &param: generic_params) {
        param->register_usage(i < generic_list.size() ? generic_list[i].get() : nullptr);
        i++;
    }
    return (int16_t) ((int16_t) total_generic_iterations(generic_params) - (int16_t) 1);
}

int16_t register_generic_usage(
        SymbolResolver& resolver,
        ASTNode* node,
        std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params,
        std::vector<std::unique_ptr<BaseType>>& generic_list
) {
    int16_t i = get_iteration_for(generic_params, generic_list);
    if(i != -1) return i;
    resolver.imported_generic[node->root_parent()] = true;
    return register_generic_usage_no_check(generic_params, generic_list);
}