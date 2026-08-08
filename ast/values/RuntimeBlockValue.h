#pragma once

#include "ast/base/Value.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/structures/Scope.h"
#include "ast/values/ValueNode.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"

class InterpretScope;

class RuntimeBlockValue : public Value {
public:
    Scope scope;

    RuntimeBlockValue(
        ASTNode* parent,
        Scope scope,
        BaseType* explicitType,
        SourceLocation loc
    ) : Value(ValueKind::RuntimeBlockValue, explicitType, loc), scope(std::move(scope)) {
    }

    Value* copy(ASTAllocator& allocator) override {
        auto cp = new (allocator.allocate<RuntimeBlockValue>()) RuntimeBlockValue(scope.parent(), Scope{scope.parent(), scope.encoded_location()}, getType(), encoded_location());
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
        return this;
    }

#ifdef COMPILER_BUILD
    llvm::AllocaInst* llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final;
    unsigned int store_in_struct(Codegen& gen, Value* parent, llvm::Value* allocated, llvm::Type* allocated_type, std::vector<llvm::Value*> idxList, unsigned int index, BaseType* expected_type) final;
    unsigned int store_in_array(Codegen& gen, Value* parent, llvm::Value* allocated, llvm::Type* allocated_type, std::vector<llvm::Value*> idxList, unsigned int index, BaseType* expected_type) final;
    llvm::Value* llvm_pointer(Codegen& gen) final;
    llvm::Value* llvm_value(Codegen& gen, BaseType* type = nullptr) final;
    void llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) final;
#endif
};
