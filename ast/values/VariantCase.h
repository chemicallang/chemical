// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/Value.h"
#include "VariantCaseVariable.h"

#pragma once

class VariantCase : public Value {
public:

    Value* parent_val;
    std::vector<VariantCaseVariable> identifier_list;
    SwitchStatement* switch_statement;

    /**
     * constructor
     */
    constexpr VariantCase(
            Value* parent_val,
            SwitchStatement* statement,
            SourceLocation location
    ) : Value(ValueKind::VariantCase, location), parent_val(parent_val), switch_statement(statement) {

    }


    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

#ifdef COMPILER_BUILD

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

#endif

};