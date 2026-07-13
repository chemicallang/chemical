// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/BaseType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "NotValue.h"
#include "BoolValue.h"
#include "NullValue.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/lab/LabBuildCompiler.h"

bool NotValue::primitive() {
    return false;
}

Value* NotValue::evaluated_value(InterpretScope &scope) {
    const auto val = value->evaluated_value(scope);
    if(!val) return nullptr;
    if(val->val_kind() == ValueKind::Bool) {
        return new (scope.allocate<BoolValue>()) BoolValue(!val->as_bool_unsafe()->value, scope.global->typeBuilder.getBoolType(), encoded_location());
    } else if(val->val_kind() == ValueKind::StructValue && scope.global && scope.global->build_compiler) {
        // Operator overload fallback: look up the not operator implementation
        auto& coreNodes = scope.global->build_compiler->coreNodes;
        auto& implsIndex = scope.global->build_compiler->implsIndex;
        auto structVal = val->as_struct_value_unsafe();
        auto ext = structVal->linked_extendable();
        if(ext) {
            auto container = ext->get_members_container();
            if(container) {
                auto overloaded = implsIndex.get_not_op_impl(coreNodes, container);
                if(overloaded) {
                    const auto prev_func = scope.global->current_func_type;
                    scope.global->current_func_type = overloaded;
                    scope.global->call_stack.emplace_back(nullptr);
                    InterpretScope fn_scope(scope.global, scope.global->allocator, scope.global);
                    std::vector<Value*> emptyArgs;
                    auto result = overloaded->call(&scope, emptyArgs, val, &fn_scope, true, this);
                    scope.global->call_stack.pop_back();
                    scope.global->current_func_type = prev_func;
                    return result;
                }
            }
        }
        scope.error("couldn't evaluate as value didn't return a boolean value or have a not operator overload", this);
        return new (scope.allocate<NullValue>()) NullValue(scope.global->typeBuilder.getNullPtrType(), encoded_location());
    } else {
        scope.error("couldn't evaluate as value didn't return a boolean value", this);
        return new (scope.allocate<NullValue>()) NullValue(scope.global->typeBuilder.getNullPtrType(), encoded_location());
    };
}

void verify_bool_ptr_condition(ASTDiagnoser& linker, BaseType* condType, SourceLocation loc);

void NotValue::determine_type(ASTDiagnoser& diagnoser, const CoreNodes& coreNodes, const ImplementationsIndex& implsIndex) {
    const auto type = getValue()->getType();
    // check if operator is overloaded
    const auto node = type->get_linked_canonical_node(true, false);
    if(node) {
        const auto container = node->get_members_container();
        if(container) {
            const auto func = implsIndex.get_not_op_impl(coreNodes, container);
            if (func == nullptr) {
                diagnoser.error(this) << "expected 'not' operator overloaded function implementation";
                setType(type);
                return;
            }
            if(func->params.size() != 1) {
                diagnoser.error(this) << "expected 'not' operator function to have a single parameter";
                setType(type);
                return;
            }
            setType(func->returnType);
            return;
        }
    }
    // normal flow
    setType(type);
    // check not value type is proper
    verify_bool_ptr_condition(diagnoser, type, encoded_location());
}