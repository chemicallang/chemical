// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/structures/Scope.h"
#include <optional>

class SwitchStatement : public ASTNode, public Value {
public:

    Value* expression;
    std::vector<Scope> scopes;
    // cases store the index of scope, if value is nullptr it means default case
    std::vector<std::pair<Value*, int>> cases;
    int defScopeInd = -1;
    bool is_value;

    constexpr SwitchStatement(
        Value* expression,
        ASTNode* parent_node,
        bool is_value,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::SwitchStmt, parent_node, location), Value(ValueKind::SwitchValue, location), expression(expression), is_value(is_value) {

    }

    SwitchStatement* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<SwitchStatement>()) SwitchStatement(
            expression,
            parent(),
            is_value,
            ASTNode::encoded_location()
        );
        stmt->cases.reserve(cases.size());
        for(auto& aCase : cases) {
            stmt->cases.emplace_back(aCase.first->copy(allocator), aCase.second);
        }
        stmt->scopes.reserve(scopes.size());
        for(auto& scope : scopes) {
            stmt->scopes.emplace_back(scope.parent(), scope.encoded_location());
            scope.copy_into(stmt->scopes.back(), allocator, stmt);
        }
        return stmt;
    }

    bool declare_and_link(SymbolResolver &linker, Value** value_ptr);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final {
        declare_and_link(linker, nullptr);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final {
        return declare_and_link(linker, &value_ptr);
    }

    Value* get_value_node();

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType *known_type() final;

    ASTNode *linked_node() final;

    inline bool has_default_case() {
        return defScopeInd != -1;
    }

    void interpret(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool last_block);

    void code_gen(Codegen &gen) {
        code_gen(gen, false);
    }

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) final;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final {
        const auto linked = linked_node();
        return linked != nullptr && linked->add_child_index(gen, indexes, name);
    }

    void code_gen(Codegen &gen, Scope* scope, unsigned int index) final;

#endif

};