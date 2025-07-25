// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/Value.h"
#include "VariantCaseVariable.h"

#pragma once

class VariantCase : public Value {
public:

    VariantMember* member;
    std::vector<VariantCaseVariable*> identifier_list;
    SwitchStatement* switch_statement;

    /**
     * constructor
     */
    constexpr VariantCase(
            VariantMember* member,
            SwitchStatement* statement,
            VoidType* voidTy,
            SourceLocation location
    ) : Value(ValueKind::VariantCase, location), member(member), switch_statement(statement) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto varCase = new (allocator.allocate<VariantCase>()) VariantCase(
            member,
            switch_statement,
            (VoidType*) getType(),
            encoded_location()
        );
        varCase->identifier_list = identifier_list;
        return varCase;
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

#endif

};