// Copyright (c) Qinetik 2024.

#include "TryCatch.h"

TryCatch::TryCatch(
        Scope tryScope,
        catch_var_type catchVar,
        std::optional<Scope> catchScope
) : tryScope(std::move(tryScope)), catchVar(std::move(catchVar)), catchScope(std::move(catchScope)){

}

#ifdef COMPILER_BUILD

void TryCatch::code_gen(Codegen &gen) {

}

#endif

void TryCatch::accept(Visitor &visitor) {
    visitor.visit(this);
}

std::string TryCatch::representation() const {
    std::string rep("try {\n");
    rep.append(tryScope.representation());
    rep.append("\n}");
    if(catchScope.has_value()) {
        rep.append(" catch ");
        if(catchVar.has_value()) {
            rep.append("(" + catchVar.value().first + ':' + catchVar.value().second->representation() + ") ");
        }
        rep.append("{\n");
        rep.append(catchScope.value().representation());
        rep.append("\n}");
    }
    return rep;
}