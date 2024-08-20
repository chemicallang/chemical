// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"

class VariantCall : public Value {
public:

    std::unique_ptr<AccessChain> chain;
    std::vector<std::unique_ptr<Value>> values;
    std::vector<std::unique_ptr<BaseType>> generic_list;

    /**
     * this will take the access chain, if has function call at least, own it's values
     */
    explicit VariantCall(std::unique_ptr<AccessChain> chain);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

};