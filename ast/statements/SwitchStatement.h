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

    VariantDefinition* getVarDefFromExpr();

    SwitchStatement* copy(ASTAllocator &allocator) override;

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) final;

    Value* get_value_node();

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType *known_type() final;

    ASTNode *linked_node() final;

    inline bool has_default_case() {
        return defScopeInd != -1;
    }

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