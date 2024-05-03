// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class StructType : public BaseType {
public:

    std::vector<std::unique_ptr<BaseType>> elem_types;

    StructType() = default;

    StructType(std::vector<std::unique_ptr<BaseType>> elem_types) : elem_types(std::move(elem_types)) {

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

    std::string representation() const override {
        std::string rep("struct {\n");
        auto i = 0;
        while (i < elem_types.size()) {
            rep.append("e" + std::to_string(i) + elem_types[i]->representation());
            i++;
        }
        rep.append("\n}");
        return rep;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) const override;

#endif

};