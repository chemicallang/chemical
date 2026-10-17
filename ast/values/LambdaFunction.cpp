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
    // Lambda bodies are lowered to MIR and executed by the MIR engine; the AST
    // statement interpreter that used to run them here has been deleted.
    if (call_scope) {
        call_scope->error("lambda body evaluation at signal resolution is not supported "
                          "(lambdas are evaluated by the MIR engine)",
                          (ASTNode*) this);
    }
    return nullptr;
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