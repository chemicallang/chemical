// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"
#include <optional>

class SwitchStatement : public ASTNode {
public:

    std::unique_ptr<Value> expression;
    std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes;
    std::optional<Scope> defScope;

    SwitchStatement(
        std::unique_ptr<Value> expression,
        std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes,
        std::optional<Scope> defScope
    );

    void accept(Visitor &visitor) override;

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool last_block);

    void code_gen(Codegen &gen) {
        code_gen(gen, false);
    }

    void code_gen(Codegen &gen, std::vector<std::unique_ptr<ASTNode>> &nodes, unsigned int index) override;

#endif

    std::string representation() const override;

};