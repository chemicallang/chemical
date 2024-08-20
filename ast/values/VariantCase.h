// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"

class VariantCaseVariable : public ASTNode {
public:

    std::string name;
    VariantCase* variant_case;

    /**
     * variant case
     */
    explicit VariantCaseVariable(std::string name, VariantCase* variant_case) : name(std::move(name)), variant_case(variant_case) {

    }

    void accept(Visitor *visitor) override {
        throw std::runtime_error("VariantCaseVariable cannot be visited, As is it always contained within a VariantCase which is visited");
    }

    void declare_and_link(SymbolResolver &linker) override;

    ASTNodeKind kind() override {
        return ASTNodeKind::VariantCaseVariable;
    }

    ASTNode* parent() override {
        return nullptr;
    }

};

class VariantCase : public Value {
public:

    std::unique_ptr<AccessChain> chain;
    std::vector<VariantCaseVariable> identifier_list;

    /**
     * this will not only take the access chain, but also find the last function call
     * and take identifiers properly
     * this also takes a diagnoser reference, so it can report errors
     */
    VariantCase(std::unique_ptr<AccessChain> chain, ASTDiagnoser& resolver);

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

};