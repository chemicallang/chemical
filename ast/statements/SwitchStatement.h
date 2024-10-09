// Copyright (c) Qinetik 2024.


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
    ASTNode* parent_node;
    bool is_value;
    CSTToken* token;

    SwitchStatement(
        Value* expression,
        ASTNode* parent_node,
        bool is_value,
        CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::SwitchValue;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::SwitchStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

    bool declare_and_link(SymbolResolver &linker, Value** value_ptr);

    void declare_and_link(SymbolResolver &linker) override {
        declare_and_link(linker, nullptr);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override {
        return declare_and_link(linker, &value_ptr);
    }

    Value* get_value_node();

    BaseType* create_type(ASTAllocator& allocator) override;

    BaseType* create_value_type(ASTAllocator& allocator) override;

    BaseType *known_type() override;

    ASTNode *linked_node() override;

    inline bool has_default_case() {
        return defScopeInd != -1;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool last_block);

    void code_gen(Codegen &gen) {
        code_gen(gen, false);
    }

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    llvm::Value * llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override {
        const auto linked = linked_node();
        return linked != nullptr && linked->add_child_index(gen, indexes, name);
    }

    void code_gen(Codegen &gen, Scope* scope, unsigned int index) override;

#endif

};