// Copyright (c) Qinetik 2024.

#include "SwitchStatement.h"
#include "ast/base/Value.h"

SwitchStatement::SwitchStatement(
        std::unique_ptr<Value> expression,
        std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes,
        std::optional<Scope> defScope
) : expression(std::move(expression)), scopes(std::move(scopes)), defScope(std::move(defScope)) {

}

void SwitchStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

#ifdef COMPILER_BUILD

void SwitchStatement::code_gen(Codegen &gen) {

}

#endif

std::string SwitchStatement::representation() const {
    std::string rep("switch {\n");

    rep.append("\n}");
    return rep;
}