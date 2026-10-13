#pragma once

#include "ast/base/Value.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/structures/Scope.h"
#include "ast/structures/CapturedComptimeVariable.h"
#include "ast/values/ValueNode.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"

class InterpretScope;

class RuntimeBlockValue : public Value {
public:
    Scope scope;

    /**
     * the comptime values captured from the enclosing comptime function's
     * scope (parameters/locals referenced inside the block's scope). the
     * runtime block value acts as a template: when evaluated_value is called,
     * a shallow copy is created and the captured values are evaluated into
     * the copy's refs
     */
    CapturedComptimeValues captured_refs;

    RuntimeBlockValue(
        ASTNode* parent,
        Scope scope,
        BaseType* explicitType,
        SourceLocation loc
    ) : Value(ValueKind::RuntimeBlockValue, explicitType, loc), scope(std::move(scope)) {
    }

    Value* copy(ASTAllocator& allocator) override {
        auto cp = new (allocator.allocate<RuntimeBlockValue>()) RuntimeBlockValue(scope.parent(), Scope{scope.parent(), scope.encoded_location()}, getType(), encoded_location());
        cp->captured_refs = captured_refs;
        scope.copy_into(cp->scope, allocator, scope.parent());
        return cp;
    }

    Value* get_stmt_expr() {
        const auto last = scope.nodes.back();
        switch (last->kind()) {
            case ASTNodeKind::AccessChainNode:
                return &last->as_access_chain_node_unsafe()->chain;
            case ASTNodeKind::ValueNode:
                return last->as_value_node_unsafe()->value;
            default:
                return nullptr;
        }
    }

    Value* evaluated_value(InterpretScope& scope) override {
        if(scope.global->interpretation_mode) {
            return this;
        }
        // the returned runtime block value references comptime variables
        // captured from the enclosing comptime function's scope
        // (CapturedComptimeVariable bridge nodes). the interpret scope where
        // the return is being interpreted is still alive, so a shallow copy of
        // the runtime block value is created (sharing the block's AST nodes)
        // and the captured values are evaluated into the copy's refs. the copy
        // is fully self-contained and can be translated by the backend after
        // the interpret scope dies. the original keeps acting as a template
        auto* cp = new (scope.allocator.allocate<RuntimeBlockValue>()) RuntimeBlockValue(
            this->scope.parent(), Scope{this->scope.nodes, this->scope.parent(), this->scope.encoded_location()}, getType(), encoded_location()
        );
        cp->captured_refs = captured_refs;
        cp->captured_refs.evaluate(scope);
        return cp;
    }

};
