// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

class DeleteStmt : public ASTNode {
public:

    bool is_array;
    std::unique_ptr<Value> identifier;
    FunctionDeclaration* free_func_linked = nullptr;

    DeleteStmt(std::unique_ptr<Value> value, bool is_array);

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};