// Copyright (c) Qinetik 2024.

#include "VariantCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"

VariantCall::VariantCall(std::unique_ptr<AccessChain> _chain) : chain(std::move(_chain)) {
    const auto func_call = chain->values.back()->as_func_call();
    if(func_call) {
        for(auto& value : func_call->values) {
            values.emplace_back(std::move(value));
        }
        for(auto& type : func_call->generic_list) {
            generic_list.emplace_back(std::move(type));
        }
        chain->values.pop_back();
    }
}

void VariantCall::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    // we've already linked chain, when variant call is created, access chain is checked, so no need to link
    for(auto& type : generic_list) {
        type->link(linker, type);
    }
    for(auto& value : values) {
        value->link(linker, value);
    }
}