// Copyright (c) Qinetik 2024.

#include "ast/base/GlobalInterpretScope.h"

using CompTimeFuncType = std::function<Value *(InterpretScope *, std::vector<std::unique_ptr<Value>> &)>;

/**
 * this takes a c++ lambda function that will run when user calls it during interpretation
 * when the lambda is evaluated, it must return a Value*, which will be the return of this function
 */
class CompTimeFuncDecl : public FunctionDeclaration {
public:

    CompTimeFuncType lambda;

    CompTimeFuncDecl(
            CompTimeFuncType lambda,
            std::string name,
            func_params params,
            std::unique_ptr<BaseType> returnType,
            bool isVariadic,
            std::optional<LoopScope> body = std::nullopt
    ) : lambda(std::move(lambda)),
        FunctionDeclaration(std::move(name), std::move(params), std::move(returnType), isVariadic, std::move(body)) {

    }

    Value * call(std::vector<std::unique_ptr<Value>> &call_params) override {
        return lambda(declarationScope, call_params);
    }

    Value *call(InterpretScope *scope, std::vector<std::unique_ptr<Value>> &call_params) override {
        return lambda(scope, call_params);
    }

};

/**
 * defines a single function
 * @param scope global scope
 * @param name name of the function
 * @param func definition of the function
 */
void define_func(GlobalInterpretScope &scope, const std::string &name, CompTimeFuncType func, bool isVariadic = false);

/**
 * this creates the normal print function
 */
CompTimeFuncType create_print(GlobalInterpretScope &global);

/**
 * defines all functions in global scope
 * these functions are interpreted functions like print and ansi_print
 */
void define_all(GlobalInterpretScope &scope);