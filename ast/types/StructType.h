// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "ast/base/BaseType.h"
#include <memory>

class StructType : public BaseType {
public:

    std::vector<std::unique_ptr<BaseType>> elem_types;

    StructType() = default;

    StructType(std::vector<std::unique_ptr<BaseType>> elem_types) : elem_types(std::move(elem_types)) {

    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Struct;
    }

    ValueType value_type() const override {
        return ValueType::Struct;
    }

    bool equals(StructType *type) const {
        if (elem_types.size() != type->elem_types.size()) return false;
        auto equal = true;
        unsigned i = 0;
        while (i < elem_types.size()) {
            if (!elem_types[i]->is_same(type->elem_types[i].get())) {
                return false;
            }
            i++;
        }
        return equal;
    }

    bool is_same(BaseType *type) const override {
        return kind() == type->kind() && equals(static_cast<StructType *>(type));
    }

    virtual BaseType *copy() const {
        auto def = new StructType();
        for (auto &elem: elem_types) {
            def->elem_types.emplace_back(elem->copy());
        }
        return def;
    }

    bool satisfies(ValueType type) const override {
        return type == ValueType::Struct;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};