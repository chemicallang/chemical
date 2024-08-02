// Copyright (c) Qinetik 2024.

#include "ASTUtils.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/FunctionCall.h"

bool chain_contains_func_call(std::vector<std::unique_ptr<Value>>& values, int start, int end) {
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

std::unique_ptr<Value> call_with_arg(FunctionDeclaration* decl, std::unique_ptr<Value> arg) {
    auto chain = std::make_unique<AccessChain>(nullptr);
    auto id = std::make_unique<VariableIdentifier>(decl->name);
    id->linked = decl;
    chain->values.emplace_back(std::move(id));
    auto imp_call = std::make_unique<FunctionCall>(std::vector<std::unique_ptr<Value>> {});
    imp_call->parent_val = chain->values[0].get();
    imp_call->values.emplace_back(std::move(arg));
    chain->values.emplace_back(std::move(imp_call));
    return chain;
}