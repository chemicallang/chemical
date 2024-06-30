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
            ASTNode* parent_node,
            std::optional<LoopScope> body = std::nullopt
    ) : lambda(std::move(lambda)),
        FunctionDeclaration(std::move(name), std::move(params), std::move(returnType), isVariadic, parent_node, std::move(body)) {

    }

    Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_params, Value *parent_val) override {
        return lambda(call_scope, call_params);
    }

    Value *call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_params, Value *parent_val, InterpretScope *fn_scope) override {
        return lambda(call_scope, call_params);
    }

};