// Copyright (c) Qinetik 2024.

#include "Typealias.h"

#ifdef COMPILER_BUILD

void TypealiasStatement::code_gen(Codegen &gen) {

}

#endif


TypealiasStatement::TypealiasStatement(
        std::unique_ptr<BaseType> from,
        std::unique_ptr<BaseType> to
) : from(std::move(from)), to(std::move(to)) {

}

void TypealiasStatement::interpret(InterpretScope &scope) {

}

void TypealiasStatement::declare_and_link(ASTLinker &linker) {

}

void TypealiasStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

std::string TypealiasStatement::representation() const {
    return "typealias " + from->representation() + " = " + to->representation();
}