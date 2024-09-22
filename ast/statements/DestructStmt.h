// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

class ExtendableMembersContainerNode;

struct DestructData {
    ExtendableMembersContainerNode* parent_node;
    FunctionDeclaration* destructor_func;
    int array_size; // -1 if not known
};

class DestructStmt : public ASTNode {
public:

    /**
     * if the statement has brackets destruct[] ptr;
     */
    bool is_array;
    /**
     * the actual identifier / access chain value destruct[array_value] ptr; <--- ptr is the one
     */
    Value* identifier;
    /**
     * array value is the one in brackets like destruct[array_value] ptr;
     */
    Value* array_value;
    /**
     * free function is linked
     */
    FunctionDeclaration* free_func_linked = nullptr;
    /**
     * parent node
     */
    ASTNode* parent_node;
    /**
     * the cst token
     */
    CSTToken* token;

    /**
     * constructor
     */
    DestructStmt(
        Value* array_value,
        Value* value,
        bool is_array,
        ASTNode* parent_node,
        CSTToken* token
    );

    CSTToken* cst_token() override {
        return token;
    }

    DestructData get_data(ASTAllocator& allocator);

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