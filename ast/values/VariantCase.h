// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "VariantCaseVariable.h"

#pragma once

class VariantCase : public Value {
public:

    std::unique_ptr<AccessChain> chain;
    std::vector<VariantCaseVariable> identifier_list;
    SwitchStatement* switch_statement;
    CSTToken* token;

    /**
     * this will not only take the access chain, but also find the last function call
     * and take identifiers properly
     * this also takes a diagnoser reference, so it can report errors
     */
    VariantCase(std::unique_ptr<AccessChain> chain, ASTDiagnoser& resolver, SwitchStatement* statement, CSTToken* token);

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::VariantCase;
    }

    bool link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

#endif

};