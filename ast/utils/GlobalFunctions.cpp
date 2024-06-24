// Copyright (c) Qinetik 2024.

#include "GlobalFunctions.h"

#include <utility>
#include "ast/types/VoidType.h"
#include "InterpretValues.h"
#include "ast/values/IntValue.h"

void define_source_stream_fns(GlobalInterpretScope &global);

void define_func(GlobalInterpretScope &scope, const std::string &name, CompTimeFuncType func, bool isVariadic) {
    auto decl = std::make_unique<CompTimeFuncDecl>(func, name, std::vector<std::unique_ptr<FunctionParam>>(), std::make_unique<VoidType>(),
                                                   isVariadic);
    std::vector<std::unique_ptr<ASTNode>> nodes;
    decl->body.emplace(LoopScope(std::move(nodes)));
    decl->interpret(scope);
    scope.global_fns[name] = std::move(decl);
}

CompTimeFuncType create_print(GlobalInterpretScope &global) {
    return [](InterpretScope *scope, std::vector<std::unique_ptr<Value>> &params) -> Value * {
        for (auto const &value: params) {
            auto paramValue = value->evaluated_value(*scope);
            if(paramValue == nullptr) {
                std::cout << "null";
            } else {
                std::cout << paramValue->representation();
            }
        }
        return nullptr;
    };
}

CompTimeFuncType create_vector(GlobalInterpretScope &global) {
    auto globalVec = global.global_vals.find("vector");
    if (globalVec == global.global_vals.end()) {
        // defining vector member function values
        std::unordered_map<std::string, MemValueFn> member_fns;
        member_fns["size"] = [](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            return new IntValue(static_cast<InterpretVectorValue*>(value)->values.size());
        };
        member_fns["push"] = [](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            if(params.empty()) return nullptr;
            static_cast<InterpretVectorValue*>(value)->values.emplace_back(params[0]->param_value(scope));
            return nullptr;
        };
        member_fns["erase"] = [](
                InterpretScope &scope,
                Value *value,
                std::vector<std::unique_ptr<Value>> &params
        ) -> Value * {
            if(params.empty()) return nullptr;
            auto& values =  static_cast<InterpretVectorValue*>(value)->values;
            values.erase(values.begin() + params[0]->as_int());
            return nullptr;
        };
        global.global_vals["vector"] = std::make_unique<MemberFnsValue>(std::move(member_fns));
    }
    return [](InterpretScope *scope, std::vector<std::unique_ptr<Value>> &params) -> Value * {
        auto& members = static_cast<MemberFnsValue*>(scope->global->global_vals["vector"].get())->members;
        auto values = std::vector<std::unique_ptr<Value>>();
        return new InterpretVectorValue(std::move(values), members);
    };
}

void define_all(GlobalInterpretScope &scope) {
    define_source_stream_fns(scope);
    define_func(scope, "vector", create_vector(scope), false);
    define_func(scope, "print", create_print(scope), true);
}