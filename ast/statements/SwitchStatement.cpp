// Copyright (c) Qinetik 2024.

#include "SwitchStatement.h"

SwitchStatement::SwitchStatement() {

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