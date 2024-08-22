// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

class DestructStmt : public ASTNode {
public:

    /**
     * if the statement has brackets destruct[] ptr;
     */
    bool is_array;
    /**
     * the actual identifier / access chain value destruct[array_value] ptr; <--- ptr is the one
     */
    std::unique_ptr<Value> identifier;
    /**
     * array value is the one in brackets like destruct[array_value] ptr;
     */
    std::unique_ptr<Value> array_value;
    /**
     * free function is linked
     */
    FunctionDeclaration* free_func_linked = nullptr;
    /**
     * parent node
     */
    ASTNode* parent_node;

    /**
     * constructor
     */
    DestructStmt(
        std::unique_ptr<Value> array_value,
        std::unique_ptr<Value> value,
        bool is_array,
        ASTNode* parent_node
    );

    ASTNode* parent() override {
        return parent_node;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::DeleteStmt;
    }

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};