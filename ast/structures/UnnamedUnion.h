// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"

class UnnamedUnion : public BaseDefMember, public VariablesContainer {
public:

    UnnamedUnion(
        std::string name
    );

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type * llvm_type(Codegen &gen) override;

#endif

};