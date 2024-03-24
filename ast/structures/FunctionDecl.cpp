// Copyright (c) Qinetik 2024.

#include "ast/base/GlobalInterpretScope.h"

void FunctionDeclaration::interpret_scope_ends(InterpretScope &scope) {
    scope.global->erase_node(name);
}

Value* FunctionDeclaration::call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_params, std::unique_ptr<InterpretScope> fn_scope) {
    if (!body.has_value()) return nullptr;
    if (params.size() != call_params.size()) {
        fn_scope->error("function " + name + " requires " + std::to_string(params.size()) + ", but given params are " +
                        std::to_string(call_params.size()));
        return nullptr;
    }
    auto i = 0;
    while (i < params.size()) {
        fn_scope->declare(params[i].name, call_params[i]->param_value(*call_scope));
        i++;
    }
    auto previous = call_scope->global->curr_node_position;
    call_scope->global->curr_node_position = 0;
    body.value().interpret(*fn_scope);
    call_scope->global->curr_node_position = previous;
    // delete all the primitive values that were copied into the function
    i--;
    while (i > -1) {
        auto itr = fn_scope->find_value_iterator(params[i].name);
        if (itr.first != itr.second.end()) {
            if (itr.first->second != nullptr && itr.first->second->primitive()) {
                delete itr.first->second;
            }
            itr.second.erase(itr.first);
        } else {
            fn_scope->error("couldn't find parameter for cleanup after function call " + params[i].name);
        }
        i--;
    }
    return interpretReturn;
}