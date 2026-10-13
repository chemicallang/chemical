// Copyright (c) Chemical Language Foundation 2025.

#include "LambdaFunction.h"

#include <memory>
#include "ast/structures/FunctionDeclaration.h"
#include "FunctionCall.h"
#include "ast/types/FunctionType.h"
#include "ast/statements/Return.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/types/VoidType.h"
#include "ast/statements/VarInit.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/base/LoopASTNode.h"
#include "ast/values/StructValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/LinkedType.h"


Value* LambdaFunction::call(
    InterpretScope *call_scope,
    ASTAllocator& func_allocator,
    Value* parent,
    std::vector<Value*>& call_args
) {
    const auto global = call_scope->global;
    // lambdas recurse on the C++ stack as well, so a lambda calling itself must be
    // stopped before the stack of the thread is exhausted, which would crash the compiler
    if(call_scope->stack_space_exhausted((ASTNode*) this)) {
        return call_scope->getNullValue();
    }
    const auto prev_func = global->current_func_type;
    global->current_func_type = this;

    InterpretScope fn_scope(global, func_allocator, global);

    auto self_param = get_self_param();
    if(self_param) {
        fn_scope.declare(self_param->name, parent);
    }

    auto i = 0u;
    while (i < call_args.size()) {
        const auto param = func_param_for_arg_at(i);
        if(!param) break;
        const auto param_val = call_args[i]->scope_value(*call_scope);
        fn_scope.declare(param->name, param_val);
        i++;
    }

    auto expected = expectedArgsSize();
    while(i < expected) {
        const auto param = func_param_for_arg_at(i);
        if (param) {
            if(param->defValue) {
                fn_scope.declare(param->name, param->defValue->scope_value(*call_scope));
            } else if(!isInVarArgs(i)) {
                call_scope->error("lambda parameter doesn't have default value, no argument exists for it", this);
            }
        }
        i++;
    }

    for(const auto cap : captureList) {
        const auto captured_val = call_scope->find_value(cap->name);
        if(captured_val) {
            fn_scope.declare(cap->name, captured_val);
        } else {
            call_scope->error(this) << "could not find captured variable '" << cap->name << "' in current scope";
        }
    }

    {
        InterpretScope body_scope(&fn_scope, fn_scope.allocator, fn_scope.global);
        body_scope.interpret(&scope);
        if(fn_scope.returnValue) {
            body_scope.returnValue = fn_scope.returnValue;
        }
    }

    // Self reference parameter (&self) should not be destructed when fn_scope ends.
    if(self_param && self_param->type && self_param->type->is_reference()) {
        auto self_it = fn_scope.values.find(self_param->name);
        if(self_it != fn_scope.values.end()) {
            self_it->second = nullptr;
        }
    }

    global->current_func_type = prev_func;
    return fn_scope.returnValue;
}

bool LambdaFunction::has_destructor_for_capture() {
    if(captureList.empty()) {
        // since user did not capture any variable, we don't need to generate a destructor
        return false;
    }
    for(const auto cap : captureList) {
        const auto ty = cap->known_type();
        if(ty && ty->get_destructor() != nullptr){
            return true;
        }
    }
    return false;
}

Value* LambdaFunctionCopy::copy(ASTAllocator& allocator) {
    const auto current = (LambdaFunction*) this;
    auto& scope = current->scope;
    const auto lambda = new (allocator.allocate<LambdaFunction>()) LambdaFunction(
        current->isVariadic(), scope.parent(), encoded_location()
    );
    for(const auto captured : current->captureList) {
        lambda->captureList.emplace_back(captured->copy(allocator));
    }
    scope.copy_into(lambda->scope, allocator, scope.parent());
    current->FunctionTypeBody::copy_into(*lambda, allocator, scope.parent());
    return lambda;
}