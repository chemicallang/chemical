// Copyright (c) Qinetik 2024.

#include "TryCatch.h"

TryCatch::TryCatch() {

}

#ifdef COMPILER_BUILD

void TryCatch::code_gen(Codegen &gen) {

}

#endif

void TryCatch::accept(Visitor &visitor) {
    visitor.visit(this);
}

std::string TryCatch::representation() const {
    std::string rep("try {} catch {}");
    return rep;
}