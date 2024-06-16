// Copyright (c) Qinetik 2024.

#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FunctionParam.h"

/**
 * this is the type of function that can be inserted into global scope
 */
typedef Value*(*CompTimeFuncType)(InterpretScope *, std::vector<std::unique_ptr<Value>> &);

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
            std::vector<std::unique_ptr<FunctionParam>> params,
            std::unique_ptr<BaseType> returnType,
            bool isVariadic,
            std::optional<LoopScope> body = std::nullopt
    ) : lambda(std::move(lambda)),
        FunctionDeclaration(std::move(name), std::move(params), std::move(returnType), isVariadic, std::move(body)) {

    }

    Value * call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_params) override {
        return lambda(call_scope, call_params);
    }

    Value *call(InterpretScope *scope, std::vector<std::unique_ptr<Value>> &call_params, InterpretScope* fn_scope) override {
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
 * this creates the vector function, which returns a vector
 */
CompTimeFuncType create_vector(GlobalInterpretScope &global);

/**
 * this creates the normal print function
 */
CompTimeFuncType create_print(GlobalInterpretScope &global);

/**
 * defines all functions in global scope
 * these functions are interpreted functions like print and vector
 */
void define_all(GlobalInterpretScope &scope);