// Copyright (c) Qinetik 2024.

#include "GlobalFunctions.h"

#include <utility>
#include "ast/types/VoidType.h"

void define_func(GlobalInterpretScope& scope, const std::string& name, CompTimeFuncType func, bool isVariadic) {
    auto decl = std::make_unique<CompTimeFuncDecl>(std::move(func), name, func_params(), std::make_unique<VoidType>(), isVariadic);
    decl->declarationScope = &scope;
    std::vector<std::unique_ptr<ASTNode>> nodes;
    decl->body.emplace(LoopScope(std::move(nodes)));
    scope.global_fns[name] = std::move(decl);
}

CompTimeFuncType create_print(GlobalInterpretScope &global) {
    return [&](InterpretScope* scope, std::vector<std::unique_ptr<Value>>& params) -> Value* {
        for (auto const &value: params) {
            auto paramValue = value->evaluated_value(*scope);
            if (paramValue != nullptr) {
                std::cout << paramValue->interpret_representation();
            } else {
                scope->error("(function call) function parameter not found : " + value->representation());
            }
        }
        return nullptr;
    };
}

CompTimeFuncType create_ansi_print(GlobalInterpretScope &global){
    return [&](InterpretScope* scope, std::vector<std::unique_ptr<Value>>& params) -> Value* {
        for (auto const &value: params) {
            auto paramValue = value->evaluated_value(*scope);
            if (paramValue != nullptr) {
                std::cout << paramValue->interpret_representation();
            } else {
                scope->error("(function call) function parameter not found : " + value->representation());
            }
        }
        return nullptr;
    };
}

void define_all(GlobalInterpretScope &scope) {
    define_func(scope, "print", create_print(scope), true);
    define_func(scope, "ansi_print", create_ansi_print(scope), true);
}