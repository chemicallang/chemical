// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

class ArrayValue : public Value {
public:

    ArrayValue(std::vector<std::unique_ptr<Value>> values, std::optional<std::unique_ptr<BaseType>> type,
               std::vector<unsigned int> sizes) : values(std::move(values)), elemType(std::move(type)),
                                                  sizes(std::move(sizes)) {
        values.shrink_to_fit();
    }

    bool primitive() override {
        return false;
    }

    inline unsigned int array_size() {
        if (sizes.empty()) {
            return values.size();
        } else {
            // TODO support multi dimensional arrays
            return sizes[0];
        }
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::AllocaInst *llvm_allocate(Codegen &gen, const std::string &identifier) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, unsigned int index) override;

#endif

    ValueType value_type() const override {
        return ValueType::Array;
    }

    Value *copy(InterpretScope &scope) override {
        std::vector<std::unique_ptr<Value>> copied_values(values.size());
        for (const auto &value: values) {
            copied_values.emplace_back(value->copy(scope));
        }
        std::vector<unsigned int> copied_sizes(sizes.size());
        std::optional<std::unique_ptr<BaseType>> copied_elem_type = std::nullopt;
        if (elemType.has_value()) {
            copied_elem_type.emplace(elemType.value()->copy());
        }
        return new ArrayValue(std::move(copied_values), std::move(copied_elem_type), sizes);
    }

    std::string representation() const override {
        std::string rep;
        rep.append(1, '[');
        if (values.empty()) {
            rep.append("]" + elemType.value()->representation() + "(");
            int i = 0;
            while (i < sizes.size()) {
                rep.append(std::to_string(sizes[i]));
                if (i < sizes.size() - 1) {
                    rep.append(", ");
                }
                i++;
            }
            rep.append(")");
        } else {
            int i = 0;
            while (i < values.size()) {
                rep.append(values[i]->representation());
                if (i != values.size() - 1) {
                    rep.append(1, ',');
                }
                i++;
            }
        }
        rep.append(1, ']');
        return rep;
    }

    std::vector<std::unique_ptr<Value>> values;

    std::optional<std::unique_ptr<BaseType>> elemType;
    std::vector<unsigned int> sizes;

#ifdef COMPILER_BUILD
    // TODO this arr value should be stored in code gen since its related to that
    llvm::AllocaInst *arr;
#endif

};