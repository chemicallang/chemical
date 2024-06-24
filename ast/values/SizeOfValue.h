// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

/**
 * will replace itself at resolution phase with the size of the given type
 */
class SizeOfValue : public Value {
private:
    std::unique_ptr<BaseType> for_type;
public:

    explicit SizeOfValue(BaseType *for_type);

    void accept(Visitor *visitor) override {
        throw std::runtime_error("A SizeOfValue cannot be visited, because it's replaced at resolution phase");
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

};