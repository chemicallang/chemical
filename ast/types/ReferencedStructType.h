// Copyright (c) Qinetik 2024.

#pragma once

#include "StructType.h"

class ReferencedStructType : public StructType {
public:

    StructDefinition* definition;
    int16_t generic_iteration;

    ReferencedStructType(StructDefinition* definition, int16_t iteration);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    VariablesContainer *variables_container() override;

    std::string struct_name() override;

    int16_t get_generic_iteration() override {
        return generic_iteration;
    }

    [[nodiscard]]
    BaseType *copy() const override;

    ASTNode *linked_node() override;

};