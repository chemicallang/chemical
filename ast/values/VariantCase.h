// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "VariantCaseVariable.h"

#pragma once

class VariantCase : public Value {
public:

    Value* parent_val;
    std::vector<VariantCaseVariable> identifier_list;
    SwitchStatement* switch_statement;

    /**
     * this will not only take the access chain, but also find the last function call
     * and take identifiers properly
     * this also takes a diagnoser reference, so it can report errors
     */
    VariantCase(
            Value* parent_val,
            std::vector<Value*>& identifier_list,
            ASTDiagnoser& resolver,
            SwitchStatement* statement,
            SourceLocation location
    ) : Value(ValueKind::VariantCase, location), parent_val(parent_val), switch_statement(statement) {

    }

    /**
     * constructor
     */
    VariantCase(
            Value* parent_val,
            SwitchStatement* statement,
            SourceLocation location
    ) : Value(ValueKind::VariantCase, location), parent_val(parent_val), switch_statement(statement) {

    }


    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

#endif

};