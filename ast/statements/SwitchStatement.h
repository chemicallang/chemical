// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/structures/Scope.h"
#include <optional>

class SwitchStatement : public ASTNode, public Value {
public:

    std::unique_ptr<Value> expression;
    std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes;
    std::optional<Scope> defScope;
    ASTNode* parent_node;
    bool is_value;

    SwitchStatement(
        std::unique_ptr<Value> expression,
        std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes,
        std::optional<Scope> defScope,
        ASTNode* parent_node,
        bool is_value
    );

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

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>* node_ptr, std::unique_ptr<Value>* value_ptr);

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override {
        declare_and_link(linker, &node_ptr, nullptr);
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override {
        declare_and_link(linker, nullptr, &value_ptr);
    }

    Value* get_value_node();

    std::unique_ptr<BaseType> create_type() override;

    std::unique_ptr<BaseType> create_value_type() override;

    BaseType *known_type() override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool last_block);

    void code_gen(Codegen &gen) {
        code_gen(gen, false);
    }

    void code_gen(Codegen &gen, Scope* scope, unsigned int index) override;

#endif

};